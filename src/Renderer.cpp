//
// Created by lupo on 04.07.26.
//

/**
 * @file Renderer.cpp
 * @brief Convert RGB frames into ANSI-formatted ASCII frames for terminal output.
 *
 * Implements colorized and grayscale rendering paths and a character lookup
 * table optimized for fast per-pixel mapping. Also contains a decoder helper
 * (decode_frames) that produces scaled RGB frames suitable for rendering.
 */

#include <AudioPlayer.hpp>
#include <Renderer.hpp>
#include <Verbose.hpp>
#include <algorithm>
#include <cmath>
#include <dataStructures.hpp>
#include <filesystem>
#include <format>
#include <string>
#include <thread>
#include <utf8.h>
#include <utility>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

void Renderer::build_char_lut() {
    DEBUG("Building character lookup table");
    const auto& charset = config.charset;
    const size_t n = charset.size();

    for (size_t i = 0; i < 256; ++i) {
        const auto index = static_cast<size_t>(static_cast<float>(i) / 255.0f * static_cast<float>(n - 1));
        utf8::utf32to8(&charset[index], &charset[index + 1], std::back_inserter(m_char_lut[i]));
    }
}

void Renderer::build_padding() {
    DEBUG("Build padding got called");
    if (int pad = config.left_pad; pad > 0) {
        DEBUG("Left padding is set");
        for (; pad > 0; --pad) { m_left_pad_str.push_back(' '); }
    } else {
        DEBUG("Left padding is disabled");
        m_left_pad_str.clear();
    }
}

Renderer::Renderer(const bool no_audio, const bool no_output, AudioPlayer* audio, const int frame_rate,
        OutputWriter* output_writer)
    : audio_(audio), m_no_new_frames_(false) {
    DEBUG("Initializing Renderer");
    // Generiert einen Lookup table für Zahlen, als strings um die nicht immer während des rendering zu generieren
    for (int i = 0; i < 256; ++i) { m_number_lut[i] = std::to_string(i); }
    build_char_lut();
    build_padding();

    no_audio_ = no_audio;
    no_output_ = no_output;
    frame_rate_ = frame_rate;
    output_writer_ = output_writer;
}

Renderer::~Renderer() {
    DEBUG("Destroying Renderer");
    DEBUG("Waiting for worker thread to finish");
    stop();
    if (worker_thread_.joinable()) { worker_thread_.join(); }
}

void Renderer::start_rendering() {
    // Implementation for starting the rendering process
    worker_thread_ = std::thread([this] {
        bool first_frame = true;
        double target_ms = 0.0;
        int frame_index = 0;

        while (!m_no_new_frames_ || !m_frame_queue.empty()) {
            DataStructures::Frame frame{.width = 0, .height = 0, .source_fps = 0.0, .data = {}};
            while (frame.width == 0 && frame.height == 0 && frame.source_fps == 0.0) {
                if (m_no_new_frames_ && m_frame_queue.empty()) { break; }
                {
                    std::lock_guard lock(m_queue_mutex);
                    if (!m_frame_queue.empty()) {
                        frame = m_frame_queue.front();
                        m_frame_queue.pop();
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            if (frame.width == 0 || frame.height == 0 || frame.source_fps <= 0.0) {
                if (m_no_new_frames_) { break; }
                continue;
            }

            if (first_frame) {
                clock_.start();
                output_writer_->set_clock(clock_);

                if (!no_output_ && !no_audio_ && audio_ != nullptr) { audio_->play(); }

                const double source_fps = frame.source_fps;

                target_ms = frame_rate_ > 0 ? 1000.0 / frame_rate_ : 1000.0 / source_fps;

                if (!no_output_) {
                    output_writer_->start();
                    while (!output_writer_->push("\033[2J\033[H")) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }
                }

                first_frame = false;
            }

            const std::string rendered = render_frame(frame);

            if (!no_output_) {
                const double expected = frame_index * target_ms;
                while (!output_writer_->push("\033[H" + rendered, expected)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }

            frame_index++;
        }
    });
}

bool Renderer::add_decoded_frame(const DataStructures::Frame& frame) {
    {
        std::lock_guard lock(m_queue_mutex);
        if (m_frame_queue.size() >= QUEUE_MAX_SIZE) { return false; }
        m_frame_queue.push(frame);
    }
    return true;
}

void Renderer::stop() {
    DEBUG("Renderer: no_new_frames got called");
    m_no_new_frames_ = true;
    if (worker_thread_.joinable()) { worker_thread_.join(); }
}

void Renderer::set_charset(const std::u32string& charset) {
    DEBUG("set_charset got called");
    this->config.charset = charset;
    // Rebuild the character lookup table
    this->build_char_lut();
}

void Renderer::set_left_pad(const int left_pad) {
    DEBUG("set_left_pad got called with pad: " + std::to_string(left_pad));
    this->config.left_pad = left_pad;
    build_padding();
}

std::string Renderer::render_frame(const DataStructures::Frame& frame) const {
    DEBUG("Renderer: render_frame got called");
    DEBUG(std::format("Rendering frame of size {}x{}", frame.width, frame.height));
    std::string output;

    if (this->config.color) {
        DEBUG("Rendering with color enabled");
        output.append(COLOR_RESET);

        uint8_t last_r = 0, last_g = 0, last_b = 0;

        // 25 ist die ungefähre anzahl in bytes die ich pro pixel brauche + 1 zur sicher heit
        output.reserve(static_cast<size_t>(frame.width * frame.height * 26 + m_left_pad_str.size() * frame.height));
        for (int y = 0; y < frame.height; ++y) {
            bool first_pixel_in_line = true;
            const DataStructures::Pixel* row = &frame.data[y * frame.width];
#ifdef DEBUG_RENDERER_MODE
            output.append(std::to_string(y));
#endif

            output.append(m_left_pad_str);
            for (int x = 0; x < frame.width; ++x) {
                const auto& pixel = row[x];

                // Änder die Ansi sequence nur, wenn sie anders ist als die vorherige oder es der erste frame pixel ist
                if (first_pixel_in_line || pixel.r != last_r || pixel.g != last_g || pixel.b != last_b) {
                    output.append(COLOR_PREFIX);
                    output.append(m_number_lut[pixel.r]);
                    output.push_back(';');
                    output.append(m_number_lut[pixel.g]);
                    output.push_back(';');
                    output.append(m_number_lut[pixel.b]);
                    output.push_back('m');

                    last_r = pixel.r;
                    last_g = pixel.g;
                    last_b = pixel.b;
                    first_pixel_in_line = false;
                }

                output.append(m_char_lut[pixel.CalculateLuminance()]);
            }
            output.append(COLOR_RESET);// Reset color at the end of each line
            output.push_back('\n');
        }
    } else {
        DEBUG("Rendering with color disabled");
        output.reserve(static_cast<size_t>(frame.width * frame.height * 12 + m_left_pad_str.size() * frame.height));
        for (int y = 0; y < frame.height; ++y) {
            const DataStructures::Pixel* row = &frame.data[y * frame.width];
            output.append(m_left_pad_str);
            for (int x = 0; x < frame.width; ++x) {
                const auto& pixel = row[x];
                output.append(m_char_lut[pixel.CalculateLuminance()]);
            }
            output.push_back('\n');
        }
    }

    return output;
}


/**
 * @brief Decode a video file and return scaled RGB frames in memory.
 * The caller can hand the result directly to the renderer.
 * @param input_path the path to the image
 * @param frame_rate the fps to sample the frames at (0 = use source fps)
 * @param width the width used as target
 * @param height the height used as target
 * @param on_frame the function to call when a frame is redy
 */
void Renderer::decode_frames(const std::filesystem::path& input_path, const int frame_rate, const int width,
        const int height, const FrameCallback& on_frame) {
    DEBUG("GenerateFrames: generate got called");
    DEBUG(std::format("Input path: {}, frame rate: {}, width: {}, height: {}", input_path.string(), frame_rate, width,
            height));

    // Keep FFmpeg resources in one place so every early return still frees them.
    struct Cleanup {
        AVPacket* pkt = av_packet_alloc();
        AVFormatContext* fmt;
        AVCodecContext* dec_ctx;
        AVFrame* frame;
        AVFrame* rgb_frame;
        SwsContext* sws;
        uint8_t* rgb_buffer;

        Cleanup() {
            fmt = nullptr;
            dec_ctx = nullptr;
            frame = nullptr;
            rgb_frame = nullptr;
            sws = nullptr;
            rgb_buffer = nullptr;
            pkt = nullptr;
        }

        ~Cleanup() {
            av_free(rgb_buffer);
            av_frame_free(&rgb_frame);
            av_frame_free(&frame);
            sws_freeContext(sws);
            avcodec_free_context(&dec_ctx);
            avformat_close_input(&fmt);
            av_packet_free(&pkt);
        }
    } cleanup;

    if (frame_rate < 0) { throw std::invalid_argument("Invalid frame rate"); }

    if (avformat_open_input(&cleanup.fmt, input_path.string().c_str(), nullptr, nullptr) < 0) {
        throw std::runtime_error("Failed to open input file");
    }

    if (avformat_find_stream_info(cleanup.fmt, nullptr) < 0) { throw std::runtime_error("Failed to read stream info"); }

    int video_stream_index = -1;
    for (unsigned int i = 0; i < cleanup.fmt->nb_streams; ++i) {
        if (cleanup.fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = static_cast<int>(i);
            break;
        }
    }

    if (video_stream_index < 0) { throw std::runtime_error("No video stream found"); }

    const AVCodecParameters* codec_par = cleanup.fmt->streams[video_stream_index]->codecpar;
    const AVCodec* dec = avcodec_find_decoder(codec_par->codec_id);
    if (!dec) { throw std::runtime_error("Failed to find decoder"); }

    cleanup.dec_ctx = avcodec_alloc_context3(dec);
    if (!cleanup.dec_ctx) { throw std::runtime_error("Failed to allocate decoder context"); }

    if (avcodec_parameters_to_context(cleanup.dec_ctx, codec_par) < 0) {
        throw std::runtime_error("Failed to copy codec parameters");
    }

    const unsigned int cpu_threads = std::max(1u, std::thread::hardware_concurrency());
    cleanup.dec_ctx->thread_count = static_cast<int>(cpu_threads);
    cleanup.dec_ctx->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;

    if (avcodec_open2(cleanup.dec_ctx, dec, nullptr) < 0) { throw std::runtime_error("Failed to open decoder"); }

    cleanup.frame = av_frame_alloc();
    cleanup.rgb_frame = av_frame_alloc();
    if (!cleanup.frame || !cleanup.rgb_frame) { throw std::runtime_error("Failed to allocate frames"); }

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
            case AV_PIX_FMT_YUVJ420P:
                return AV_PIX_FMT_YUV420P;
            case AV_PIX_FMT_YUVJ422P:
                return AV_PIX_FMT_YUV422P;
            case AV_PIX_FMT_YUVJ444P:
                return AV_PIX_FMT_YUV444P;
            case AV_PIX_FMT_YUVJ440P:
                return AV_PIX_FMT_YUV440P;
            default:
                return format;
        }
    };

    if (target_w <= 0 && target_h <= 0) { throw std::invalid_argument("Invalid target dimensions"); }

    if (target_h <= 0) {
        target_h =
                std::max(1, static_cast<int>(std::llround(static_cast<double>(src_h) * static_cast<double>(target_w) /
                                                          (static_cast<double>(src_w) * char_aspect))));
    } else if (target_w <= 0) {
        target_w =
                std::max(1, static_cast<int>(std::llround(static_cast<double>(src_w) * static_cast<double>(target_h) *
                                                          char_aspect / static_cast<double>(src_h))));
    }

    cleanup.sws = sws_getContext(cleanup.dec_ctx->width, cleanup.dec_ctx->height,
            normalize_pixel_format(cleanup.dec_ctx->pix_fmt), target_w, target_h, AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR,
            nullptr, nullptr, nullptr);
    if (!cleanup.sws) { throw std::runtime_error("Failed to create scaling context"); }

    // Check if the colorspace can be converted
    if (sws_setColorspaceDetails(cleanup.sws, sws_getCoefficients(SWS_CS_DEFAULT), 0,
                sws_getCoefficients(SWS_CS_DEFAULT), 1, 0, 1 << 16, 1 << 16) < 0) {
        throw std::runtime_error("Failed to configure colorspace conversion");
    }

    const int rgb_buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, target_w, target_h, 1);
    if (rgb_buffer_size < 0) { throw std::runtime_error("Failed to allocate RGB buffer"); }

    cleanup.rgb_buffer = static_cast<uint8_t*>(av_malloc(rgb_buffer_size));
    if (!cleanup.rgb_buffer) { throw std::runtime_error("Failed to allocate RGB memory"); }

    if (av_image_fill_arrays(cleanup.rgb_frame->data, cleanup.rgb_frame->linesize, cleanup.rgb_buffer, AV_PIX_FMT_RGB24,
                target_w, target_h, 1) < 0) {
        throw std::runtime_error("Failed to bind RGB buffer to frame");
    }

    AVStream* video_stream = cleanup.fmt->streams[video_stream_index];
    double source_fps = av_q2d(av_guess_frame_rate(cleanup.fmt, video_stream, nullptr));
    if (source_fps <= 0.0) { source_fps = av_q2d(video_stream->avg_frame_rate); }
    if (source_fps <= 0.0) { source_fps = static_cast<double>(frame_rate); }

    const double effective_fps = (frame_rate > 0) ? static_cast<double>(frame_rate) : source_fps;

    if (effective_fps <= 0.0) { throw std::runtime_error("Could not determine a valid frame rate"); }

    const std::size_t frame_step = std::max<std::size_t>(1, std::llround(source_fps / effective_fps));

    // Convert each decoded frame from the source pixel format into packed RGB24.
    auto append_frame = [&](const AVFrame* source_frame) {
        // scale the frame to the target height and width
        sws_scale(cleanup.sws, source_frame->data, source_frame->linesize, 0, cleanup.dec_ctx->height,
                cleanup.rgb_frame->data, cleanup.rgb_frame->linesize);

        DataStructures::Frame output_frame;
        output_frame.width = target_w;
        output_frame.height = target_h;
        output_frame.data.resize(
                static_cast<std::size_t>(target_w) * static_cast<std::size_t>(target_h));// Reserve space in the vector
        output_frame.source_fps = effective_fps;

        // write the data in the Frame
        for (int y = 0; y < target_h; ++y) {
            const uint8_t* row =
                    cleanup.rgb_frame->data[0] + static_cast<std::size_t>(y) * cleanup.rgb_frame->linesize[0];
            std::memcpy(&output_frame.data[static_cast<std::size_t>(y) * target_w], row,
                    static_cast<std::size_t>(target_w) * 3);
        }

        on_frame(std::move(output_frame));
    };

    // Decode packets, convert the chosen frames, and keep only the sampled result.
    cleanup.pkt = av_packet_alloc();
    std::size_t decoded_frame_index = 0;
    while (av_read_frame(cleanup.fmt, cleanup.pkt) >= 0) {
        if (cleanup.pkt->stream_index == video_stream_index) {
            if (avcodec_send_packet(cleanup.dec_ctx, cleanup.pkt) >= 0) {
                while (avcodec_receive_frame(cleanup.dec_ctx, cleanup.frame) >= 0) {
                    if (decoded_frame_index % frame_step == 0) { append_frame(cleanup.frame); }
                    ++decoded_frame_index;
                }
            }
        }
        av_packet_unref(cleanup.pkt);
    }

    // Flush the decoder to process any remaining frames.
    if (avcodec_send_packet(cleanup.dec_ctx, nullptr) >= 0) {
        while (avcodec_receive_frame(cleanup.dec_ctx, cleanup.frame) >= 0) {
            if (decoded_frame_index % frame_step == 0) { append_frame(cleanup.frame); }
            ++decoded_frame_index;
        }
    }
}
