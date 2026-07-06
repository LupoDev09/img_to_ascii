//
// Created by lupo on 06.07.26.
//

#ifndef IMG_TO_ASCII_AUDIOPLAYER_H
#define IMG_TO_ASCII_AUDIOPLAYER_H
#pragma once
#include <chrono>
#include <filesystem>
#include <miniaudio.h>
#include <stdexcept>
#include <string>
#include <fstream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}


class AudioPlayer {
public:
    AudioPlayer() {
        // Engine initialisieren (Cross-platform Backend automatisch)
        if (ma_engine_init(nullptr, &engine) != MA_SUCCESS) {
            throw std::runtime_error("Failed to init audio engine");
        }
    }

    ~AudioPlayer() {
        // Ressourcen sauber freigeben (verhindert Memory leaks)
        ma_engine_uninit(&engine);
    }

    void load(const std::string& input_file, const std::string& output_file_path) {
        if (!std::filesystem::exists(input_file)) {
            get_audio_file(input_file, output_file_path);
        }
        audio_file = output_file_path;
    }

    void play() {
        if (audio_file.empty()) {
            throw std::runtime_error("No audio file loaded");
        }

        // Startet Audio asynchron im Hintergrund
        ma_engine_play_sound(&engine, audio_file.c_str(), nullptr);

        start_time = std::chrono::steady_clock::now();
        playing = true;
    }

    void stop() {
        ma_engine_stop(&engine);
        playing = false;
    }

    void deleteAudioFile() const {
        if (!audio_file.empty()) {
            std::filesystem::remove(audio_file);
        }
    }

    [[nodiscard]] double get_time_ms() const {
        // Liefert grobe Playback-Zeitbasis für Sync
        if (!playing) return 0.0;

        const auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(now - start_time).count();
    }

private:
    static void get_audio_file(const std::string& input_file, const std::string& output_file_path) {
        AVFormatContext* format_ctx = nullptr;

        // Öffnet die Eingabedatei (Video/Audio Container)
        if (avformat_open_input(&format_ctx, input_file.c_str(), nullptr, nullptr) != 0) {
            throw std::runtime_error("Failed to open input file");
        }

        // Liest Stream-Infos (Audio/Video Tracks erkennen)
        if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
            avformat_close_input(&format_ctx);
            throw std::runtime_error("Failed to find stream info");
        }

        int audio_stream_index = -1;

        // Sucht den Audio-Stream im Container
        for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
            if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                audio_stream_index = i;
                break;
            }
        }

        if (audio_stream_index == -1) {
            avformat_close_input(&format_ctx);
            throw std::runtime_error("No audio stream found");
        }

        AVCodecParameters* codecpar = format_ctx->streams[audio_stream_index]->codecpar;

        // Decoder für Audio holen
        const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
        if (!codec) {
            avformat_close_input(&format_ctx);
            throw std::runtime_error("Unsupported audio codec");
        }

        AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(codec_ctx, codecpar);

        if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
            avcodec_free_context(&codec_ctx);
            avformat_close_input(&format_ctx);
            throw std::runtime_error("Failed to open codec");
        }

        // WAV Output Datei
        std::ofstream out(output_file_path, std::ios::binary);

        if (!out.is_open()) {
            throw std::runtime_error("Failed to open output file");
        }

        // Minimal WAV Header (wird später korrigiert)
        auto write_wav_header = [&](const int sample_rate, const int channels) {
            // RIFF Header
            out.write("RIFF", 4);
            int32_t chunk_size = 0;
            out.write(reinterpret_cast<char*>(&chunk_size), 4);
            out.write("WAVE", 4);

            // fmt chunk
            out.write("fmt ", 4);
            int32_t subchunk1_size = 16;
            int16_t audio_format = 1; // PCM
            int16_t num_channels = channels;
            int32_t sr = sample_rate;
            int16_t bits_per_sample = 16;
            int32_t byte_rate = sr * channels * bits_per_sample / 8;
            int16_t block_align = channels * bits_per_sample / 8;

            out.write(reinterpret_cast<char*>(&subchunk1_size), 4);
            out.write(reinterpret_cast<char*>(&audio_format), 2);
            out.write(reinterpret_cast<char*>(&num_channels), 2);
            out.write(reinterpret_cast<char*>(&sr), 4);
            out.write(reinterpret_cast<char*>(&byte_rate), 4);
            out.write(reinterpret_cast<char*>(&block_align), 2);
            out.write(reinterpret_cast<char*>(&bits_per_sample), 2);

            // data chunk
            out.write("data", 4);
            int32_t data_size = 0;
            out.write(reinterpret_cast<char*>(&data_size), 4);
        };

        write_wav_header(codec_ctx->sample_rate, codec_ctx->ch_layout.nb_channels);

        AVPacket* packet = av_packet_alloc();
        AVFrame* frame = av_frame_alloc();

        // Audio decode loop
        while (av_read_frame(format_ctx, packet) >= 0) {
            if (packet->stream_index == audio_stream_index) {

                if (avcodec_send_packet(codec_ctx, packet) == 0) {
                    while (avcodec_receive_frame(codec_ctx, frame) == 0) {

                        // Rohdaten (PCM)
                        //int data_size = av_get_bytes_per_sample(codec_ctx->sample_fmt);

                        for (int i = 0; i < frame->nb_samples; i++) {
                            for (int ch = 0; ch < codec_ctx->ch_layout.nb_channels; ch++) {
                                int16_t sample = reinterpret_cast<int16_t *>(frame->data[ch])[i];
                                out.write(reinterpret_cast<char*>(&sample), sizeof(int16_t));
                            }
                        }
                    }
                }
            }

            av_packet_unref(packet);
        }

        // Cleanup
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);

        out.close();
    }

    ma_engine engine{};  // Audio Backend (SDL/OpenAL/etc unter der Haube)
    std::string audio_file;

    std::chrono::steady_clock::time_point start_time;
    bool playing = false;
};

#endif// IMG_TO_ASCII_AUDIOPLAYER_H
