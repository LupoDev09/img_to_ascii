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

    void load(const std::string& file) {
        audio_file = file;
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
    ma_engine engine{};  // Audio Backend (SDL/OpenAL/etc unter der Haube)
    std::string audio_file;

    std::chrono::steady_clock::time_point start_time;
    bool playing = false;
};


#endif// IMG_TO_ASCII_AUDIOPLAYER_H
