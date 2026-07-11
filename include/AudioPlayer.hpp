//
// Created by lupo on 06.07.26.
//

#ifndef IMG_TO_ASCII_AUDIOPLAYER_H
#define IMG_TO_ASCII_AUDIOPLAYER_H
#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <miniaudio.h>
#include <stdexcept>
#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

/**
 * @class AudioPlayer
 * @brief Extracts and plays audio from video files with synchronized timing.
 * 
 * Decodes audio from various video formats (via FFmpeg), converts to standardized PCM format,
 * writes to WAV file, and plays back with precise timing for video synchronization.
 * 
 * Handles:
 * - Audio extraction from video containers (MP4, MKV, AVI, etc.)
 * - Format conversion to stereo 44.1kHz S16 PCM
 * - WAV file creation with proper headers
 * - Playback via miniaudio backend
 * - Video-audio synchronization via elapsed time tracking
 * - Files without audio streams (graceful degradation)
 */
class AudioPlayer {
public:
    /**
     * @brief Initialize audio engine.
     * @throws std::runtime_error if audio engine initialization fails
     * 
     * Automatically selects the appropriate audio backend for the platform.
     */
    AudioPlayer() {
        if (ma_engine_init(nullptr, &engine) != MA_SUCCESS) {
            throw std::runtime_error("Failed to init audio engine");
        }
    }

    /**
     * @brief Clean up audio resources.
     * 
     * Stops playback and uninitializes the audio engine.
     * Safe to call even if audio is not currently playing.
     */
    ~AudioPlayer() {
        stop();
        ma_engine_uninit(&engine);
    }

    /**
     * @brief Extract audio from a video file and prepare for playback.
     * 
     * Decodes audio from the input video, converts to standard format (Stereo, 44.1kHz, S16),
     * and writes to a WAV file. Handles files without audio gracefully.
     * 
     * @param input_file Path to video file containing audio
     * @param output_file_path Path where to write the extracted WAV file
     * 
     * @throws std::runtime_error if input file doesn't exist or audio extraction fails
     * 
     * Any existing file at output_file_path is automatically removed and replaced.
     */
    void load(const std::string& input_file, const std::string& output_file_path) {
        if (!std::filesystem::exists(input_file)) {
            throw std::runtime_error("Input file does not exist");
        }

        if (std::filesystem::exists(output_file_path)) {
            std::filesystem::remove(output_file_path);
        }

        get_audio_file(input_file, output_file_path);
        audio_file = output_file_path;
    }

    /**
     * @brief Start audio playback.
     * 
     * Begins playing the loaded audio file synchronized with external timing.
     * Sets up timing reference for get_time_ms() to track playback position.
     * 
     * @throws std::runtime_error if no audio file is loaded or playback fails
     * 
     * Safe to call multiple times (subsequent calls have no effect).
     */
    void play() {
        if (file_has_no_audio) {
            return;
        }
        if (audio_file.empty()) {
            throw std::runtime_error("No audio file loaded");
        }

        // Initialize sound with full playback control
        if (ma_sound_init_from_file(&engine, audio_file.c_str(), 0, nullptr, nullptr, &sound) != MA_SUCCESS) {
            throw std::runtime_error("Failed to load sound");
        }

        ma_sound_start(&sound);

        start_time = std::chrono::steady_clock::now();
        playing = true;
    }

    /**
     * @brief Stop audio playback.
     * 
     * Halts playback and frees associated resources.
     * Safe to call when audio is not playing.
     */
    void stop() {
        if (playing && !file_has_no_audio) {
            ma_sound_stop(&sound);
            ma_sound_uninit(&sound);
            playing = false;
        }
    }

    /**
     * @brief Delete the extracted WAV audio file.
     * 
     * Removes the temporary WAV file created during load().
     * Safe to call even if the file doesn't exist.
     */
    void deleteAudioFile() const {
        if (!audio_file.empty()) {
            std::filesystem::remove(audio_file);
        }
    }

    /**
     * @brief Get the current playback position in milliseconds.
     * @return Elapsed time since play() was called, in milliseconds
     * 
     * Returns 0 if audio is not currently playing.
     * Used to synchronize video frames with audio during playback.
     */
    [[nodiscard]] double get_time_ms() const {
        if (!playing) return 0.0;

        const auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(now - start_time).count();
    }

private:
    /**
     * @brief Extract and convert audio from video file to WAV format.
     *
     * Decodes audio stream from video, resamples to Stereo 44.1kHz S16,
     * and writes to WAV file with proper headers. Handles files without
     * audio by setting file_has_no_audio flag.
     *
     * Process:
     * 1. Open video file with FFmpeg
     * 2. Find audio stream
     * 3. Create resampler to target format
     * 4. Decode frames and resample
     * 5. Write to WAV file with corrected headers
     * 6. Clean up all FFmpeg resources
     *
     * @param input_file Source video file
     * @param output_file_path Destination WAV file path
     * @throws std::runtime_error on file, I/O or codec errors
     */
    void get_audio_file(const std::string &input_file, const std::string &output_file_path) {
        Cleanup cleanup;

        // Open video file
        AVFormatContext *raw_format_ctx = nullptr;
        if (avformat_open_input(&raw_format_ctx, input_file.c_str(), nullptr, nullptr) != 0) {
            throw std::runtime_error("Failed to open input file");
        }
        // Ownership an unique_ptr übergeben
        cleanup.format_ctx.reset(raw_format_ctx);

        // Die stream informationen sammeln
        if (avformat_find_stream_info(cleanup.format_ctx.get(), nullptr) < 0) {
            throw std::runtime_error("Failed to find stream info");
        }

        // Find audio stream
        int audio_stream_index = -1;
        for (unsigned int i = 0; i < cleanup.format_ctx->nb_streams; i++) {
            if (cleanup.format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                audio_stream_index = static_cast<int>(i);
                break;
            }
        }

        // Wenn es keinen audiostream gibt, gibt einfach den bool dafür auf true setzen und abbrechen
        if (audio_stream_index == -1) {
            file_has_no_audio = true;
            return;
        }


        AVCodecParameters *codecpar = cleanup.format_ctx->streams[audio_stream_index]->codecpar;
        const AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
        if (!codec) {
            throw std::runtime_error("Unsupported codec");
        }


        // Codec Context erstellen
        cleanup.codec_ctx.reset(avcodec_alloc_context3(codec));

        if (!cleanup.codec_ctx) {
            throw std::runtime_error("Failed to alloc codec context");
        }

        if (avcodec_parameters_to_context(cleanup.codec_ctx.get(), codecpar) < 0) {
            throw std::runtime_error("Failed copying codec parameters");
        }

        if (avcodec_open2(cleanup.codec_ctx.get(), codec, nullptr) < 0) {
            throw std::runtime_error("Failed to open codec");
        }

        // Target format: stereo PCM S16 at 44.1kHz
        AVChannelLayout out_ch_layout;
        av_channel_layout_default(&out_ch_layout, 2);
        AVChannelLayout in_ch_layout = cleanup.codec_ctx->ch_layout;
        SwrContext *raw_swr = nullptr;

        int ret = swr_alloc_set_opts2(&raw_swr, &out_ch_layout, AV_SAMPLE_FMT_S16,44100, &in_ch_layout,
        cleanup.codec_ctx->sample_fmt,cleanup.codec_ctx->sample_rate,0, nullptr);

                if (ret < 0) {
            throw std::runtime_error("Failed to allocate swr context");
        }
        cleanup.swr.reset(raw_swr);


        if (swr_init(cleanup.swr.get()) < 0) {
            throw std::runtime_error("Failed to init swr");
        }

        std::ofstream out(output_file_path, std::ios::binary);
        if (!out.is_open()) {
            throw std::runtime_error("Failed to open output file");
        }
        auto write_wav_header = [&](const int sample_rate, const int channels) {
            int32_t chunk_size = 0;
            out.write("RIFF", 4);
            out.write(reinterpret_cast<char *>(&chunk_size), 4);
            out.write("WAVE", 4);
            out.write("fmt ", 4);

            int32_t subchunk1_size = 16;
            int16_t audio_format = 1;
            auto num_channels = static_cast<int16_t>(channels);
            int32_t sr = sample_rate;
            int16_t bits_per_sample = 16;

            int32_t byte_rate = sr * channels * bits_per_sample / 8;

            auto block_align = static_cast<int16_t>(channels * bits_per_sample / 8);

            out.write(reinterpret_cast<char *>(&subchunk1_size), 4);
            out.write(reinterpret_cast<char *>(&audio_format), 2);
            out.write(reinterpret_cast<char *>(&num_channels), 2);
            out.write(reinterpret_cast<char *>(&sr), 4);
            out.write(reinterpret_cast<char *>(&byte_rate), 4);
            out.write(reinterpret_cast<char *>(&block_align), 2);
            out.write(reinterpret_cast<char *>(&bits_per_sample), 2);

            out.write("data", 4);

            int32_t data_size = 0;
            out.write(reinterpret_cast<char *>(&data_size), 4);
        };

        write_wav_header(44100, 2);

        uint8_t *out_buffer = nullptr;
        int out_linesize;

        // Decode loop
        while (av_read_frame(cleanup.format_ctx.get(), cleanup.packet.get()) >= 0) {
            if (cleanup.packet->stream_index == audio_stream_index) {
                if (avcodec_send_packet(cleanup.codec_ctx.get(), cleanup.packet.get()) == 0) {
                    while (avcodec_receive_frame(cleanup.codec_ctx.get(), cleanup.frame.get()) == 0) {
                        av_samples_alloc(&out_buffer, &out_linesize, 2, cleanup.frame->nb_samples, AV_SAMPLE_FMT_S16, 0);

                        int samples = swr_convert(
                                cleanup.swr.get(), &out_buffer, cleanup.frame->nb_samples,
                                cleanup.frame->data, cleanup.frame->nb_samples);

                        out.write(reinterpret_cast<char *>(out_buffer), static_cast<std::streamsize>(samples * 2 * sizeof(int16_t)));
                        av_freep(&out_buffer);
                    }
                }
            }
            av_packet_unref(cleanup.packet.get());
        }



        // Flush decoder
        avcodec_send_packet(cleanup.codec_ctx.get(), nullptr);
        while (avcodec_receive_frame(cleanup.codec_ctx.get(), cleanup.frame.get()) == 0) {
            av_samples_alloc(
                    &out_buffer,
                    &out_linesize,
                    2,
                    cleanup.frame->nb_samples,
                    AV_SAMPLE_FMT_S16,
                    0);

            int samples = swr_convert(
                    cleanup.swr.get(),
                    &out_buffer,
                    cleanup.frame->nb_samples,
                    cleanup.frame->data,
                    cleanup.frame->nb_samples);


            out.write(reinterpret_cast<char *>(out_buffer), static_cast<std::streamsize>(samples * 2 * sizeof(int16_t)));
            av_freep(&out_buffer);
        }



        // WAV Header korrigieren
        auto file_size = static_cast<std::streamoff>(out.tellp());
        auto data_size = static_cast<int32_t>(file_size - 44);
        auto chunk_size = static_cast<int32_t>(file_size - 8);

        out.seekp(4);
        out.write(reinterpret_cast<char *>(&chunk_size), 4);
        out.seekp(40);
        out.write(reinterpret_cast<char *>(&data_size), 4);
        out.close();
    }
    ma_engine engine{};                     ///< Miniaudio engine instance
    ma_sound sound{};                       ///< Sound currently playing

    std::string audio_file;                 ///< Path to loaded WAV file
    std::chrono::steady_clock::time_point start_time; ///< Playback start time reference
    bool playing = false;                   ///< Current playback state
    bool file_has_no_audio = false;         ///< Flag if source video has no audio stream

    struct Cleanup {

        using FormatPtr = std::unique_ptr<
            AVFormatContext,
            decltype([](AVFormatContext* ctx) {
                if (ctx) {
                    avformat_close_input(&ctx);
                }
            })
        >;

        using CodecPtr = std::unique_ptr<
            AVCodecContext,
            decltype([](AVCodecContext* ctx) {
                if (ctx) {
                    avcodec_free_context(&ctx);
                }
            })
        >;

        using FramePtr = std::unique_ptr<
            AVFrame,
            decltype([](AVFrame* frame) {
                if (frame) {
                    av_frame_free(&frame);
                }
            })
        >;

        using PacketPtr = std::unique_ptr<
            AVPacket,
            decltype([](AVPacket* packet) {
                if (packet) {
                    av_packet_free(&packet);
                }
            })
        >;

        using SwrPtr = std::unique_ptr<
            SwrContext,
            decltype([](SwrContext* swr) {
                if (swr) {
                    swr_free(&swr);
                }
            })
        >;


        FormatPtr format_ctx{nullptr};
        CodecPtr codec_ctx{nullptr};
        FramePtr frame{av_frame_alloc()};
        PacketPtr packet{av_packet_alloc()};
        SwrPtr swr{nullptr};
    };
};

#endif // IMG_TO_ASCII_AUDIOPLAYER_H