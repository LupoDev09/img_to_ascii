//
// Created by lupo on 06.07.26.
//

#ifndef IMG_TO_ASCII_AUDIOPLAYER_H
#define IMG_TO_ASCII_AUDIOPLAYER_H

#include <AudioExtractor.hpp>
#include <chrono>
#include <filesystem>
#include <miniaudio.h>
#include <string>

class AudioPlayer {
public:
    /**
     * @brief Inits this class and its resources. The player is not ready to play audio until load() is called.
     */
    AudioPlayer();

    /**
     * @brief Destroys this class and its resources.
     * If audio is playing, it will be stopped and unloaded. Frees the memory from the extractor
     */
    ~AudioPlayer();

    // Keine Kopien oder Zuweisungen
    AudioPlayer(const AudioPlayer &) = delete;
    AudioPlayer &operator=(const AudioPlayer &) = delete;
    AudioPlayer(AudioPlayer &&) = delete;
    AudioPlayer &operator=(AudioPlayer &&) = delete;

    /**
     * @brief loads the audio file from the given path. If a file is already loaded, it will be unloaded first.
     * @param path the path to the audio file to load.
     */
    void load(const std::string &path);

    /// @brief plays the loaded file (if any).
    void play();

    /// @brief stops the playback (if any).
    void stop();

    /// @brief free the resources used by the player (if any).
    void unload();

    /**
     * @brief returns the time that passed since the playback started in milliseconds. If no audio is playing, it returns 0.
     * @return the time that passed since the playback started
     */
    [[nodiscard]] double get_time_ms() const;

    /**
     * @brief returns true if the audio playback is currently playing, false otherwise.
     * @return if the audio playback is playing
     */
    [[nodiscard]] bool is_playing() const;

    /**
     * @brief returns if something is loaded in the player. If no audio is loaded, it returns false.
     * @return if something is loaded
     */
    [[nodiscard]] bool is_loaded() const;

private:
    // Loads the Audio track from the file and prepares it for playback.
    AudioExtractor audio_extractor;

    ma_sound sound = {};
    ma_engine engine = {};
    ma_audio_buffer audioBuffer = {};


    // The bools do what they say
    bool playing = false;
    bool is_something_loaded = false;
    std::chrono::steady_clock::time_point start_time; // The start time of the playback
};
#endif // IMG_TO_ASCII_AUDIOPLAYER_H