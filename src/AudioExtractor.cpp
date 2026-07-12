//
// Created by lupo on 12.07.26.
//

#include "../include/AudioExtractor.hpp"

#include "Verbose.hpp"

#include <format>
#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

AudioExtractor::AudioExtractor(const std::string& filepath) {
    if (!filepath.empty()) {
        DEBUG(std::format("AudioExtractor: Loading audio from file: {}", filepath));
        extractAudio(filepath);
    }
}

AudioExtractor::~AudioExtractor() = default;

bool AudioExtractor::loadFile(const std::string &filepath) {
    try {
        extractAudio(filepath);
        return true;
    } catch (std::exception& e) {
        std::cerr << std::format("[ERROR] Failed to load audio from {}\n", filepath);
        DEBUG(std::format("[ERROR] Failed to load audio from {}: {}\n", filepath, e.what()));
        return false;
    }
}

bool AudioExtractor::hasAudio() const {
    DEBUG(std::format("AudioExtractor: hasAudio() called, audioFound = {}", audioFound));
    return audioFound;
}

const uint8_t* AudioExtractor::getAudioData() const {
    DEBUG(std::format("AudioExtractor: getAudioData() called"));
    if (audioBuffer.empty()) {
        DEBUG("AudioExtractor: audioBuffer is empty");
        return nullptr;
    }
    DEBUG(std::format("AudioExtractor: Returning audio data, size = {}", audioBuffer.size()));
    return audioBuffer.data();
}

bool AudioExtractor::freeAudioData() {
    DEBUG("AudioExtractor: freeAudioData() called");
    try {
        audioBuffer.clear();
        audioBuffer.shrink_to_fit();
        sampleRate = 0;
        channels = 0;
        sampleFmt = AV_SAMPLE_FMT_NONE;
        audioFound = false;
        DEBUG("AudioExtractor: Audio data freed successfully");
        return true;
    } catch (std::exception& e) {
        std::cerr << std::format("[ERROR] AudioExtractor: Error freeing audio data: {}\n", e.what());
        return false;
    }
}


size_t AudioExtractor::getAudioDataSize() const {
    DEBUG(std::format("AudioExtractor: getAudioDataSize() called, size = {}", audioBuffer.size()));
    return audioBuffer.size();
}

int AudioExtractor::getSampleRate() const {
    DEBUG(std::format("AudioExtractor: getSampleRate() called, rate = {}", sampleRate));
    return sampleRate;
}

int AudioExtractor::getChannels() const {
    DEBUG(std::format("AudioExtractor: getChannels() called, channels = {}", channels));
    return channels;
}

AVSampleFormat AudioExtractor::getSampleFormat() const {
    DEBUG(std::format("AudioExtractor: getSampleFormat() called, format = {}", sampleFmt));
    return sampleFmt;
}

void AudioExtractor::extractAudio(const std::string& filepath) {
    DEBUG("AudioExtractor: extractAudio() got called with filepath = {}", filepath);

    // FFmpeg-Strukturen initialisieren
    AVFormatContext* fmtCtx = nullptr;
    AVCodecContext* codecCtx = nullptr;
    SwrContext* swrCtx = nullptr;
    AVPacket* pkt = nullptr;
    AVFrame* frame = nullptr;
    int audioStreamIndex = -1;

    // Eingabe öffnen
    if (avformat_open_input(&fmtCtx, filepath.c_str(), nullptr, nullptr) < 0) {
        throw std::runtime_error("AudioExtractor: Could not open input file: " + filepath);
    }

    // Stream-Informationen abrufen
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        avformat_close_input(&fmtCtx);
        throw std::runtime_error("AudioExtractor: Could not retrieve stream information");
    }

    // Besten Audiostream finden
    audioStreamIndex = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audioStreamIndex < 0) {
        // Keine Audiospur vorhanden – kein Fehler, einfach leer bleiben
        avformat_close_input(&fmtCtx);
        DEBUG("AudioExtractor: No audio stream found");
        return;
    }

    // Codec-Parameter des Audiostreams
    AVCodecParameters * codecPar = fmtCtx->streams[audioStreamIndex]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec) {
        avformat_close_input(&fmtCtx);
        throw std::runtime_error("AudioExtractor: Unsupported audio codec");
    }

    // Codec-Kontext anlegen und öffnen
    codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        avformat_close_input(&fmtCtx);
        throw std::runtime_error("AudioExtractor: Failed to allocate codec context");
    }
    if (avcodec_parameters_to_context(codecCtx, codecPar) < 0) {
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmtCtx);
        throw std::runtime_error("AudioExtractor: Failed to copy codec parameters");
    }
    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmtCtx);
        throw std::runtime_error("AudioExtractor: Failed to open audio codec");
    }

    // Resampler konfigurieren: Zielformat = signed 16-bit, interleaved, gleiche Sample-Rate + Kanäle
    const AVChannelLayout outChLayout = codecCtx->ch_layout;
    constexpr AVSampleFormat outSampleFmt = AV_SAMPLE_FMT_S16;
    const int outSampleRate = codecCtx->sample_rate;

    if (swr_alloc_set_opts2(&swrCtx,
                        &outChLayout, outSampleFmt, outSampleRate,
                        &codecCtx->ch_layout, codecCtx->sample_fmt, codecCtx->sample_rate,
                        0, nullptr) < 0) {
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmtCtx);
        throw std::runtime_error("AudioExtractor: Failed to allocate resampler");
                        }
    if (swr_init(swrCtx) < 0) {
        swr_free(&swrCtx);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmtCtx);
        throw std::runtime_error("AudioExtractor: Failed to initialize resampler");
    }

    // Datenstrukturen für das Lesen
    pkt = av_packet_alloc();
    frame = av_frame_alloc();
    if (!pkt || !frame) {
        swr_free(&swrCtx);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmtCtx);
        throw std::runtime_error("AudioExtractor: Failed to allocate frames/packets");
    }

    // Hauptschleife: Pakete lesen, decodieren, resamplen und im Speicher sammeln
    while (av_read_frame(fmtCtx, pkt) >= 0) {
        if (pkt->stream_index == audioStreamIndex) {
            if (avcodec_send_packet(codecCtx, pkt) == 0) {
                while (true) {
                    int ret = avcodec_receive_frame(codecCtx, frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    if (ret < 0) {
                        // Fehler beim Dekodieren – Abbruch mit Fehler
                        av_packet_unref(pkt);
                        av_frame_free(&frame);
                        av_packet_free(&pkt);
                        swr_free(&swrCtx);
                        avcodec_free_context(&codecCtx);
                        avformat_close_input(&fmtCtx);
                        throw std::runtime_error("AudioExtractor: Error during audio decoding");;
                    }

                    // Zielpuffer für resamplete Daten anlegen
                    int outSamples = frame->nb_samples;
                    uint8_t* outData = nullptr;
                    int outLinesize = 0;
                    if (av_samples_alloc(&outData, &outLinesize, outChLayout.nb_channels,
                                         outSamples, outSampleFmt, 0) < 0) {
                        av_frame_unref(frame);
                        continue;
                    }

                    // Resampling
                    int convertedSamples = swr_convert(swrCtx, &outData, outSamples,
                                                       (const uint8_t**)frame->data, frame->nb_samples);
                    if (convertedSamples < 0) {
                        av_freep(&outData);
                        av_frame_unref(frame);
                        continue;
                    }

                    // Daten in den Vektor anhängen
                    int bytesPerSample = av_get_bytes_per_sample(outSampleFmt);
                    int dataSize = convertedSamples * outChLayout.nb_channels * bytesPerSample;
                    const uint8_t* dataPtr = outData;
                    audioBuffer.insert(audioBuffer.end(), dataPtr, dataPtr + dataSize);

                    av_freep(&outData);
                    av_frame_unref(frame);
                }
            }
        }
        av_packet_unref(pkt);
    }

    // Decoder leeren (Flush)
    avcodec_send_packet(codecCtx, nullptr);
    while (true) {
        int ret = avcodec_receive_frame(codecCtx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        if (ret < 0) break;

        int outSamples = frame->nb_samples;
        uint8_t* outData = nullptr;
        int outLinesize = 0;
        if (av_samples_alloc(&outData, &outLinesize, outChLayout.nb_channels,
                             outSamples, outSampleFmt, 0) < 0) {
            av_frame_unref(frame);
            continue;
        }

        int convertedSamples = swr_convert(swrCtx, &outData, outSamples,
                                           (const uint8_t**)frame->data, frame->nb_samples);
        if (convertedSamples > 0) {
            int bytesPerSample = av_get_bytes_per_sample(outSampleFmt);
            int dataSize = convertedSamples * outChLayout.nb_channels * bytesPerSample;
            const uint8_t* dataPtr = outData;
            audioBuffer.insert(audioBuffer.end(), dataPtr, dataPtr + dataSize);
        }
        av_freep(&outData);
        av_frame_unref(frame);
    }

    // Attribute setzen
    sampleRate = outSampleRate;
    channels   = outChLayout.nb_channels;
    sampleFmt  = outSampleFmt;
    audioFound = true;

    // Aufräumen
    av_frame_free(&frame);
    av_packet_free(&pkt);
    swr_free(&swrCtx);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&fmtCtx);
}