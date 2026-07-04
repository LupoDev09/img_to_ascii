//
// Created by lupo on 04.07.26.
//

#include <GenerateFrames.h>

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

#include <dataStructures.h>

/**
 * Generiert Frames aus einer Video- oder GIF-Datei mit FFmpeg
 * @param input_path Der Pfad zur Eingabedatei (Video oder GIF)
 * @param frame_rate Die Anzahl der Frames pro Sekunde zum Extrahieren
 * @param width Die Zielbreite der generierten Frames
 * @param height Die Zielhöhe der generierten Frames
 */
std::vector<DataStructures::Frame> GenerateFrames::generate(const std::filesystem::path &input_path, const int frame_rate, const int width, const int height) {
    std::vector<DataStructures::Frame> frames;

    // Keep FFmpeg resources in one place so every early return still frees them.
    struct Cleanup {
        AVFormatContext* fmt = nullptr;
        AVCodecContext* dec_ctx = nullptr;
        AVFrame* frame = nullptr;
        AVFrame* rgb_frame = nullptr;
        SwsContext* sws = nullptr;
        uint8_t* rgb_buffer = nullptr;

        ~Cleanup() {
            av_free(rgb_buffer);
            av_frame_free(&rgb_frame);
            av_frame_free(&frame);
            sws_freeContext(sws);
            avcodec_free_context(&dec_ctx);
            avformat_close_input(&fmt);
        }
    } cleanup;

    if (frame_rate <= 0 || width <= 0 || height <= 0) {
        std::cerr << "Invalid output dimensions or frame rate" << std::endl;
        return {};
    }

    if (avformat_open_input(&cleanup.fmt, input_path.string().c_str(), nullptr, nullptr) < 0) {
        std::cerr << "Failed to open input file" << std::endl;
        return {};
    }

    if (avformat_find_stream_info(cleanup.fmt, nullptr) < 0) {
        std::cerr << "Failed to read stream info" << std::endl;
        return {};
    }

    int video_stream_index = -1;
    for (unsigned int i = 0; i < cleanup.fmt->nb_streams; ++i) {
        if (cleanup.fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = static_cast<int>(i);
            break;
        }
    }

    if (video_stream_index < 0) {
        std::cerr << "No video stream found" << std::endl;
        return {};
    }

    const AVCodecParameters* codec_par = cleanup.fmt->streams[video_stream_index]->codecpar;
    const AVCodec* dec = avcodec_find_decoder(codec_par->codec_id);
    if (!dec) {
        std::cerr << "Failed to find decoder" << std::endl;
        return {};
    }

    cleanup.dec_ctx = avcodec_alloc_context3(dec);
    if (!cleanup.dec_ctx) {
        std::cerr << "Failed to allocate decoder context" << std::endl;
        return {};
    }

    if (avcodec_parameters_to_context(cleanup.dec_ctx, codec_par) < 0) {
        std::cerr << "Failed to copy codec parameters" << std::endl;
        return {};
    }

    if (avcodec_open2(cleanup.dec_ctx, dec, nullptr) < 0) {
        std::cerr << "Failed to open decoder" << std::endl;
        return {};
    }

    cleanup.frame = av_frame_alloc();
    cleanup.rgb_frame = av_frame_alloc();
    if (!cleanup.frame || !cleanup.rgb_frame) {
        std::cerr << "Failed to allocate frames" << std::endl;
        return {};
    }

    cleanup.sws = sws_getContext(
        cleanup.dec_ctx->width,
        cleanup.dec_ctx->height,
        cleanup.dec_ctx->pix_fmt,
        width,
        height,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        nullptr,
        nullptr,
        nullptr
    );
    if (!cleanup.sws) {
        std::cerr << "Failed to create scaling context" << std::endl;
        return {};
    }

    const int rgb_buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
    if (rgb_buffer_size < 0) {
        std::cerr << "Failed to allocate RGB buffer" << std::endl;
        return {};
    }

    cleanup.rgb_buffer = static_cast<uint8_t*>(av_malloc(rgb_buffer_size));
    if (!cleanup.rgb_buffer) {
        std::cerr << "Failed to allocate RGB memory" << std::endl;
        return {};
    }

    if (av_image_fill_arrays(
            cleanup.rgb_frame->data,
            cleanup.rgb_frame->linesize,
            cleanup.rgb_buffer,
            AV_PIX_FMT_RGB24,
            width,
            height,
            1
        ) < 0) {
        std::cerr << "Failed to bind RGB buffer to frame" << std::endl;
        return {};
    }

    AVStream* video_stream = cleanup.fmt->streams[video_stream_index];
    double source_fps = av_q2d(av_guess_frame_rate(cleanup.fmt, video_stream, nullptr));
    if (source_fps <= 0.0) {
        source_fps = av_q2d(video_stream->avg_frame_rate);
    }
    if (source_fps <= 0.0) {
        source_fps = static_cast<double>(frame_rate);
    }

    const std::size_t frame_step = std::max<std::size_t>(
        1,
        std::llround(source_fps / static_cast<double>(frame_rate))
    );

    // Convert each decoded frame from the source pixel format into packed RGB24.
    auto append_frame = [&](const AVFrame * source_frame) {
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
        output_frame.width = width;
        output_frame.height = height;
        output_frame.data.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
        output_frame.source_fps = source_fps;

        for (int y = 0; y < height; ++y) {
            const uint8_t* row = cleanup.rgb_frame->data[0] + static_cast<std::size_t>(y) * cleanup.rgb_frame->linesize[0];
            for (int x = 0; x < width; ++x) {
                const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
                const std::size_t rgb_index = static_cast<std::size_t>(x) * 3;
                output_frame.data[index] = {row[rgb_index], row[rgb_index + 1], row[rgb_index + 2]};
            }
        }

        frames.push_back(std::move(output_frame));
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

    avcodec_send_packet(cleanup.dec_ctx, nullptr);
    while (avcodec_receive_frame(cleanup.dec_ctx, cleanup.frame) >= 0) {
        if (decoded_frame_index % frame_step == 0) {
            append_frame(cleanup.frame);
        }
        ++decoded_frame_index;
    }

    return frames;
}