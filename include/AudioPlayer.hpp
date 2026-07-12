//
// Created by lupo on 06.07.26.
//

#ifndef IMG_TO_ASCII_AUDIOPLAYER_H
#define IMG_TO_ASCII_AUDIOPLAYER_H

#include <chrono>
#include <filesystem>
#include <miniaudio.h>
#include <string>
#include <AudioExtractor.hpp>

class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();

    // Keine Kopien oder Zuweisungen
    AudioPlayer(const AudioPlayer &) = delete;
    AudioPlayer &operator=(const AudioPlayer &) = delete;
    AudioPlayer(AudioPlayer &&) = delete;
    AudioPlayer &operator=(AudioPlayer &&) = delete;

    /// @brief Load audio from a file and prepare for playback.
    /// @param path Path to the audio file to load.
    void load(const std::string &path);

    /// @brief plays the loaded file (if any).
    void play();

    /// @brief stops the playback (if any).
    void stop();

    /// @brief free the resources used by the player (if any).
    void unload();

    [[nodiscard]] double get_time_ms() const;

    [[nodiscard]] bool is_playing() const;
    [[nodiscard]] bool is_loaded() const;

private:
    AudioExtractor audio_extractor;

    ma_sound sound = {};
    ma_engine engine = {};
    ma_audio_buffer audioBuffer = {};


    bool playing = false;            // Represents if something is playing
    bool is_something_loaded = false;// Represents if something is loaded
    std::chrono::steady_clock::time_point start_time;
};
#endif // IMG_TO_ASCII_AUDIOPLAYER_H