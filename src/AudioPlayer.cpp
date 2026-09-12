//
// Created by lupo on 12.07.26.
//

/**
 * @file AudioPlayer.cpp
 * @brief Thin wrapper around miniaudio to play PCM buffers produced by AudioExtractor.
 *
 * Responsibilities:
 * - Initialize and shut down the miniaudio engine
 * - Accept raw PCM buffers (S16 interleaved) and expose them as ma_sound
 * - Track playback state and provide simple timing information
 */
#include <AudioPlayer.hpp>
#include <Verbose.hpp>

/**
 * @brief Construct and initialize the audio engine.
 *
 * Initializes the miniaudio engine used for playback. Throws on failure.
 */
AudioPlayer::AudioPlayer() {
    DEBUG("Initializing audio engine");
    if (ma_engine_init(nullptr, &engine) != MA_SUCCESS) { throw std::runtime_error("Failed to init audio engine"); }
    DEBUG("Audio engine initialized successfully");
}

/**
 * @brief Stop playback and release audio engine resources.
 *
 * Ensures any playing sound is stopped and associated buffers are freed before
 * uninitializing the audio engine.
 */
AudioPlayer::~AudioPlayer() {
    DEBUG("Destroying audio engine");
    stop();
    unload();
    ma_engine_uninit(&engine);
    if (audio_extractor.freeAudioData()) {
        DEBUG("Audio data freed successfully");
    } else {
        DEBUG("Audio data free failed");
    }
    DEBUG("Audio engine uninitialized successfully");
}

/**
 * @brief Load PCM data from the given file into a miniaudio buffer and create a sound.
 *
 * The function uses AudioExtractor to obtain raw PCM (S16) data, initializes
 * a ma_audio_buffer and constructs a ma_sound that can be started/stopped.
 * Throws std::runtime_error on failure.
 *
 * @param path Path to input media file (used by AudioExtractor)
 */
void AudioPlayer::load(const std::string& path) {
    DEBUG(std::format("Loading audio file: {}", path));
    unload();// Für den Fall das was geladen war, das Freigeben

    if (audio_extractor.loadFile(path) == false) { throw std::runtime_error("Failed to load audio"); }
    DEBUG("Audio file loaded successfully");

    if (!audio_extractor.getAudioData()) {
        is_something_loaded = false;
        return;// Keine Audiodaten, also nichts zu tun
    }
    DEBUG("Audio data available");

    // 2. Format für Miniaudio aus den Eigenschaften des Extractor
    constexpr ma_format format = ma_format_s16;// wir haben AV_SAMPLE_FMT_S16 als Ziel
    const ma_uint32 channels = audio_extractor.getChannels();

    // 3. Audiodaten in den ma_audio_buffer kopieren
    const ma_audio_buffer_config config = ma_audio_buffer_config_init(format, channels,
            audio_extractor.getAudioDataSize() / (channels * ma_get_bytes_per_sample(format)),// Anzahl Frames
            audio_extractor.getAudioData(),
            nullptr// keine eigene Allokationsfunktion
    );
    DEBUG("Audio buffer config initialized");

    if (ma_audio_buffer_init(&config, &audioBuffer) != MA_SUCCESS) {
        throw std::runtime_error("Failed to init audio buffer");
    }
    DEBUG("Audio buffer initialized successfully");

    // 4. Sound-Objekt mit dieser Datenquelle verbinden
    if (ma_sound_init_from_data_source(&engine, &audioBuffer, 0, nullptr, &sound) != MA_SUCCESS) {
        ma_audio_buffer_uninit(&audioBuffer);
        throw std::runtime_error("Failed to init sound");
    }
    DEBUG("Sound buffer initialized successfully");

    is_something_loaded = true;
}

/**
 * @brief Start playback of the loaded sound.
 *
 * If no audio is loaded this is a no-op. Marks the internal playing flag
 * and records the playback start time for timing queries.
 */
void AudioPlayer::play() {
    if (is_something_loaded) {
        DEBUG("Starting audio playback");
        ma_sound_start(&sound);
        playing = true;
        start_time = std::chrono::steady_clock::now();
        DEBUG("Playback started successfully");
    }
}

/**
 * @brief Stop (pause) playback of the current sound.
 *
 * The playback position is not explicitly reset; this call stops the sound
 * and updates the internal state to reflect that playback is not active.
 */
void AudioPlayer::stop() {
    if (is_something_loaded) {
        DEBUG("Stopping audio playback");
        ma_sound_stop(&sound);
        // Setzt Position nicht zurück, aber Wiedergabe pausiert
        playing = false;
        DEBUG("Playback stopped successfully");
    }
}

/**
 * @brief Unload the current sound and free associated buffers.
 *
 * After this call the player contains no loaded audio and is ready to load a
 * different source. Safe to call even when nothing is loaded.
 */
void AudioPlayer::unload() {
    if (is_something_loaded) {
        DEBUG("Unloading audio playback");
        ma_sound_uninit(&sound);
        ma_audio_buffer_uninit(&audioBuffer);
        is_something_loaded = false;
        playing = false;
        DEBUG("Audio playback unloaded successfully");
    }
}

bool AudioPlayer::is_loaded() const {
    DEBUG("Checking if audio is loaded: " + std::string(is_something_loaded ? "true" : "false"));
    return this->is_something_loaded;
}

bool AudioPlayer::is_playing() const {
    DEBUG("Checking if audio is playing: " + std::string(playing ? "true" : "false"));
    return this->playing;
}

/**
 * @brief Return playback time in milliseconds since play() was called.
 *
 * If playback is not active returns 0.0. The value is computed using steady_clock
 * to avoid issues with system clock adjustments.
 *
 * @return elapsed playback time in milliseconds or 0 if not playing
 */
double AudioPlayer::get_time_ms() const {
    DEBUG("get_time_ms got called");
    if (!playing) return 0.0;

    const auto now = std::chrono::steady_clock::now();
    DEBUG("Getting audio time: " + std::to_string(std::chrono::duration<double, std::milli>(now - start_time).count()));
    return std::chrono::duration<double, std::milli>(now - start_time).count();
}