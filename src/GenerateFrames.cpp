//
// Created by lupo on 04.07.26.
//

#include <GenerateFrames.hpp>
#include <Verbose.hpp>
#include <dataStructures.hpp>


extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <filesystem>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <thread>
#include <format>

/**
 * @brief Decode a video file and return scaled RGB frames in memory.
 * The caller can hand the result directly to the renderer.
 * @param input_path the path to the image
 * @param frame_rate the fps to sample the frames at (0 = use source fps)
 * @param width the width used as target
 * @param height the height used as target
 * @param on_frame the function to call when a frame is redy
 */
void GenerateFrames::generate(const std::filesystem::path &input_path, const int frame_rate, const int width, const int height, const FrameCallback &on_frame) {
    DEBUG("GenerateFrames: generate got called");
    DEBUG(std::format("Input path: {}, frame rate: {}, width: {}, height: {}", input_path.string(), frame_rate, width, height));

    // Keep FFmpeg resources in one place so every early return still frees them.
    struct Cleanup {
        AVFormatContext* fmt;
        AVCodecContext*  dec_ctx;
        AVFrame*         frame;
        AVFrame*         rgb_frame;
        SwsContext*      sws;
        uint8_t*         rgb_buffer;

        Cleanup() {
            fmt = nullptr;
            dec_ctx = nullptr;
            frame = nullptr;
            rgb_frame = nullptr;
            sws = nullptr;
            rgb_buffer = nullptr;
        }

        ~Cleanup() {
            av_free(rgb_buffer);
            av_frame_free(&rgb_frame);
            av_frame_free(&frame);
            sws_freeContext(sws);
            avcodec_free_context(&dec_ctx);
            avformat_close_input(&fmt);
        }
    } cleanup;

    if (frame_rate < 0) {
        throw std::invalid_argument("Invalid frame rate");
    }

    if (avformat_open_input(&cleanup.fmt, input_path.string().c_str(), nullptr, nullptr) < 0) {
        throw std::runtime_error("Failed to open input file");
    }

    if (avformat_find_stream_info(cleanup.fmt, nullptr) < 0) {
        throw std::runtime_error("Failed to read stream info");
    }

    int video_stream_index = -1;
    for (unsigned int i = 0; i < cleanup.fmt->nb_streams; ++i) {
        if (cleanup.fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = static_cast<int>(i);
            break;
        }
    }

    if (video_stream_index < 0) {
        throw std::runtime_error("No video stream found");
    }

    const AVCodecParameters* codec_par = cleanup.fmt->streams[video_stream_index]->codecpar;
    const AVCodec* dec = avcodec_find_decoder(codec_par->codec_id);
    if (!dec) {
        throw std::runtime_error("Failed to find decoder");
    }

    cleanup.dec_ctx = avcodec_alloc_context3(dec);
    if (!cleanup.dec_ctx) {
        throw std::runtime_error("Failed to allocate decoder context");
    }

    if (avcodec_parameters_to_context(cleanup.dec_ctx, codec_par) < 0) {
        throw std::runtime_error("Failed to copy codec parameters");
    }

    const unsigned int cpu_threads = std::max(1u, std::thread::hardware_concurrency());
    cleanup.dec_ctx->thread_count = static_cast<int>(cpu_threads);
    cleanup.dec_ctx->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;

    if (avcodec_open2(cleanup.dec_ctx, dec, nullptr) < 0) {
        throw std::runtime_error("Failed to open decoder");
    }

    cleanup.frame = av_frame_alloc();
    cleanup.rgb_frame = av_frame_alloc();
    if (!cleanup.frame || !cleanup.rgb_frame) {
        throw std::runtime_error("Failed to allocate frames");
    }

    // Compute target dimensions if one of them is zero to preserve aspect ratio.
    int target_w = width;
    int target_h = height;

    const int src_w = cleanup.dec_ctx->width;
    const int src_h = cleanup.dec_ctx->height;
    // Approximate character aspect ratio (height / width). Adjust if output looks squashed.
    // Common terminal fonts have char height ≈ 2 * width.
    constexpr double char_aspect = 2.0;

    auto normalize_pixel_format = [](const AVPixelFormat format) {
        switch (format) {
            case AV_PIX_FMT_YUVJ420P: return AV_PIX_FMT_YUV420P;
            case AV_PIX_FMT_YUVJ422P: return AV_PIX_FMT_YUV422P;
            case AV_PIX_FMT_YUVJ444P: return AV_PIX_FMT_YUV444P;
            case AV_PIX_FMT_YUVJ440P: return AV_PIX_FMT_YUV440P;
            default: return format;
        }
    };

    if (target_w <= 0 && target_h <= 0) {
        throw std::invalid_argument("Invalid target dimensions");
    }

    if (target_h <= 0) {
        target_h = std::max(1, static_cast<int>(std::llround(static_cast<double>(src_h) * static_cast<double>(target_w) / (static_cast<double>(src_w) * char_aspect))));
    } else if (target_w <= 0) {
        target_w = std::max(1, static_cast<int>(std::llround(static_cast<double>(src_w) * static_cast<double>(target_h) * char_aspect / static_cast<double>(src_h))));
    }

    cleanup.sws = sws_getContext(
        cleanup.dec_ctx->width,
        cleanup.dec_ctx->height,
        normalize_pixel_format(cleanup.dec_ctx->pix_fmt),
        target_w,
        target_h,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        nullptr,
        nullptr,
        nullptr
    );
    if (!cleanup.sws) {
        throw std::runtime_error("Failed to create scaling context");
    }

    // Check if the colorspace can be converted
    if (sws_setColorspaceDetails( cleanup.sws, sws_getCoefficients(SWS_CS_DEFAULT), 0,
        sws_getCoefficients(SWS_CS_DEFAULT), 1, 0, 1 << 16, 1 << 16) < 0) {
        throw std::runtime_error("Failed to configure colorspace conversion");
    }

    const int rgb_buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, target_w, target_h, 1);
    if (rgb_buffer_size < 0) {
        throw std::runtime_error("Failed to allocate RGB buffer");
    }

    cleanup.rgb_buffer = static_cast<uint8_t*>(av_malloc(rgb_buffer_size));
    if (!cleanup.rgb_buffer) {
        throw std::runtime_error("Failed to allocate RGB memory");
    }

    if (av_image_fill_arrays(
            cleanup.rgb_frame->data,
            cleanup.rgb_frame->linesize,
            cleanup.rgb_buffer,
            AV_PIX_FMT_RGB24,
            target_w,
            target_h,
            1
        ) < 0) {
        throw std::runtime_error("Failed to bind RGB buffer to frame");
    }

    AVStream* video_stream = cleanup.fmt->streams[video_stream_index];
    double source_fps = av_q2d(av_guess_frame_rate(cleanup.fmt, video_stream, nullptr));
    if (source_fps <= 0.0) {
        source_fps = av_q2d(video_stream->avg_frame_rate);
    }
    if (source_fps <= 0.0) {
        source_fps = static_cast<double>(frame_rate);
    }

    const double effective_fps = (frame_rate > 0)
    ? static_cast<double>(frame_rate)
    : source_fps;

    if (effective_fps <= 0.0) {
        throw std::runtime_error("Could not determine a valid frame rate");
    }

    const std::size_t frame_step = std::max<std::size_t>(
        1,
        std::llround(source_fps / effective_fps)
    );

    // Convert each decoded frame from the source pixel format into packed RGB24.
    auto append_frame = [&](const AVFrame * source_frame) {
        // scale the frame to the target height and width
        sws_scale(
            cleanup.sws,
            source_frame->data,
            source_frame->linesize,
            0,
            cleanup.dec_ctx->height,
            cleanup.rgb_frame->data,
            cleanup.rgb_frame->linesize
        );

        DataStructures::Frame output_frame;
        output_frame.width = target_w;
        output_frame.height = target_h;
        output_frame.data.resize(static_cast<std::size_t>(target_w) * static_cast<std::size_t>(target_h)); // Reserve space in the vector
        output_frame.source_fps = effective_fps;

        // write the data in the Frame
        for (int y = 0; y < target_h; ++y) {
            const uint8_t* row = cleanup.rgb_frame->data[0] + static_cast<std::size_t>(y) * cleanup.rgb_frame->linesize[0];
            for (int x = 0; x < target_w; ++x) {
                const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(target_w) + static_cast<std::size_t>(x);
                const std::size_t rgb_index = static_cast<std::size_t>(x) * 3;
                output_frame.data[index] = {row[rgb_index], row[rgb_index + 1], row[rgb_index + 2]};
            }
        }

        on_frame(std::move(output_frame));
    };

    // Decode packets, convert the chosen frames, and keep only the sampled result.
    AVPacket pkt;
    std::size_t decoded_frame_index = 0;
    while (av_read_frame(cleanup.fmt, &pkt) >= 0) {
        if (pkt.stream_index == video_stream_index) {
            if (avcodec_send_packet(cleanup.dec_ctx, &pkt) >= 0) {
                while (avcodec_receive_frame(cleanup.dec_ctx, cleanup.frame) >= 0) {
                    if (decoded_frame_index % frame_step == 0) {
                        append_frame(cleanup.frame);
                    }
                    ++decoded_frame_index;
                }
            }
        }
        av_packet_unref(&pkt);
    }

    // Flush the decoder to process any remaining frames.
    if (avcodec_send_packet(cleanup.dec_ctx, nullptr) >= 0) {
        while (avcodec_receive_frame(cleanup.dec_ctx, cleanup.frame) >= 0) {
            if (decoded_frame_index % frame_step == 0) {
                append_frame(cleanup.frame);
            }
            ++decoded_frame_index;
        }
    }
}
