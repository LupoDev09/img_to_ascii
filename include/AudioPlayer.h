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

class AudioPlayer {
public:
    AudioPlayer() {
        // Initialisiert Audio Engine (Backend automatisch gewählt)
        if (ma_engine_init(nullptr, &engine) != MA_SUCCESS) {
            throw std::runtime_error("Failed to init audio engine");
        }
    }

    ~AudioPlayer() {
        stop(); // Sicherheit: Sound stoppen bevor Engine zerstört wird
        ma_engine_uninit(&engine);
    }

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

    void play() {
        if (audio_file.empty()) {
            throw std::runtime_error("No audio file loaded");
        }

        // Sound initialisieren (gibt dir Kontrolle über Stop etc.)
        if (ma_sound_init_from_file(&engine, audio_file.c_str(), 0, nullptr, nullptr, &sound) != MA_SUCCESS) {
            throw std::runtime_error("Failed to load sound");
        }

        ma_sound_start(&sound);

        start_time = std::chrono::steady_clock::now();
        playing = true;
    }

    void stop() {
        if (playing) {
            ma_sound_stop(&sound);
            ma_sound_uninit(&sound);
            playing = false;
        }
    }

    void deleteAudioFile() const {
        if (!audio_file.empty()) {
            std::filesystem::remove(audio_file);
        }
    }

    [[nodiscard]] double get_time_ms() const {
        if (!playing) return 0.0;

        const auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(now - start_time).count();
    }

private:
    static void get_audio_file(const std::string& input_file, const std::string& output_file_path) {
        AVFormatContext* format_ctx = nullptr;

        // Datei öffnen
        if (avformat_open_input(&format_ctx, input_file.c_str(), nullptr, nullptr) != 0) {
            throw std::runtime_error("Failed to open input file");
        }

        if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
            avformat_close_input(&format_ctx);
            throw std::runtime_error("Failed to find stream info");
        }

        // Audio Stream finden
        int audio_stream_index = -1;
        for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
            if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                audio_stream_index = static_cast<int>(i);
                break;
            }
        }

        if (audio_stream_index == -1) {
            avformat_close_input(&format_ctx);
            throw std::runtime_error("No audio stream found");
        }

        AVCodecParameters* codecpar = format_ctx->streams[audio_stream_index]->codecpar;

        const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
        if (!codec) {
            avformat_close_input(&format_ctx);
            throw std::runtime_error("Unsupported codec");
        }

        AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
        if (!codec_ctx) {
            avformat_close_input(&format_ctx);
            throw std::runtime_error("Failed to alloc codec context");
        }

        avcodec_parameters_to_context(codec_ctx, codecpar);

        if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
            avcodec_free_context(&codec_ctx);
            avformat_close_input(&format_ctx);
            throw std::runtime_error("Failed to open codec");
        }

        // Ziel: immer sauberes PCM (S16, Stereo, 44100Hz)
        SwrContext* swr = nullptr;

        AVChannelLayout out_ch_layout;
        av_channel_layout_default(&out_ch_layout, 2); // Stereo

        AVChannelLayout in_ch_layout = codec_ctx->ch_layout;

        swr_alloc_set_opts2(
            &swr,
            &out_ch_layout,
            AV_SAMPLE_FMT_S16,
            44100,

            &in_ch_layout,
            codec_ctx->sample_fmt,
            codec_ctx->sample_rate,

            0, nullptr
        );

        if (!swr) {
            throw std::runtime_error("Failed to allocate swr context");
        }
        swr_init(swr);

        std::ofstream out(output_file_path, std::ios::binary);
        if (!out.is_open()) {
            throw std::runtime_error("Failed to open output file");
        }

        // WAV Header schreiben (Platzhalter)
        auto write_wav_header = [&](int sample_rate, int channels) {
            out.write("RIFF", 4);
            int32_t chunk_size = 0;
            out.write(reinterpret_cast<char*>(&chunk_size), 4);
            out.write("WAVE", 4);

            out.write("fmt ", 4);
            int32_t subchunk1_size = 16;
            int16_t audio_format = 1;
            auto num_channels = static_cast<int16_t>(channels);
            int32_t sr = sample_rate;
            int16_t bits_per_sample = 16;
            int32_t byte_rate = sr * channels * bits_per_sample / 8;
            auto block_align = static_cast<int16_t>(channels * bits_per_sample / 8);

            out.write(reinterpret_cast<char*>(&subchunk1_size), 4);
            out.write(reinterpret_cast<char*>(&audio_format), 2);
            out.write(reinterpret_cast<char*>(&num_channels), 2);
            out.write(reinterpret_cast<char*>(&sr), 4);
            out.write(reinterpret_cast<char*>(&byte_rate), 4);
            out.write(reinterpret_cast<char*>(&block_align), 2);
            out.write(reinterpret_cast<char*>(&bits_per_sample), 2);

            out.write("data", 4);
            int32_t data_size = 0;
            out.write(reinterpret_cast<char*>(&data_size), 4);
        };

        write_wav_header(44100, 2);

        AVPacket* packet = av_packet_alloc();
        AVFrame* frame = av_frame_alloc();

        uint8_t* out_buffer = nullptr;
        int out_linesize;

        // Decode Loop
        while (av_read_frame(format_ctx, packet) >= 0) {
            if (packet->stream_index == audio_stream_index) {

                if (avcodec_send_packet(codec_ctx, packet) == 0) {
                    while (avcodec_receive_frame(codec_ctx, frame) == 0) {

                        av_samples_alloc(&out_buffer, &out_linesize, 2,
                                         frame->nb_samples, AV_SAMPLE_FMT_S16, 0);

                        int samples = swr_convert(
                            swr,
                            &out_buffer,
                            frame->nb_samples,
                            (const uint8_t**)frame->data,
                            frame->nb_samples
                        );

                        // Interleaved schreiben
                        out.write(reinterpret_cast<char*>(out_buffer),
                                  samples * 2 * sizeof(int16_t));

                        av_freep(&out_buffer);
                    }
                }
            }
            av_packet_unref(packet);
        }

        // Decoder flush (wichtig für letzte Frames)
        avcodec_send_packet(codec_ctx, nullptr);
        while (avcodec_receive_frame(codec_ctx, frame) == 0) {
            av_samples_alloc(&out_buffer, &out_linesize, 2,
                             frame->nb_samples, AV_SAMPLE_FMT_S16, 0);

            int samples = swr_convert(
                swr,
                &out_buffer,
                frame->nb_samples,
                (const uint8_t**)frame->data,
                frame->nb_samples
            );

            out.write(reinterpret_cast<char*>(out_buffer),
                      samples * 2 * sizeof(int16_t));

            av_freep(&out_buffer);
        }

        // WAV Header korrigieren
        auto file_size = static_cast<std::streamoff>(out.tellp());

        auto data_size = static_cast<int32_t>(file_size - 44);
        auto chunk_size = static_cast<int32_t>(file_size - 8);

        out.seekp(4);
        out.write(reinterpret_cast<char*>(&chunk_size), 4);

        out.seekp(40);
        out.write(reinterpret_cast<char*>(&data_size), 4);

        // Cleanup
        swr_free(&swr);
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);

        out.close();
    }

    ma_engine engine{};
    ma_sound sound{}; // wichtig: wir speichern den Sound jetzt!

    std::string audio_file;

    std::chrono::steady_clock::time_point start_time;
    bool playing = false;
};

#endif // IMG_TO_ASCII_AUDIOPLAYER_H