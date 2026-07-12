//
// Created by lupo on 12.07.26.
//
#include <AudioPlayer.hpp>
#include <Verbose.hpp>

AudioPlayer::AudioPlayer() {
    DEBUG("Initializing audio engine");
    if (ma_engine_init(nullptr, &engine) != MA_SUCCESS) {
        throw std::runtime_error("Failed to init audio engine");
    }
    DEBUG("Audio engine initialized successfully");
}

AudioPlayer::~AudioPlayer() {
    DEBUG("Destroying audio engine");
    stop();
    unload();
    ma_engine_uninit(&engine);
    DEBUG("Audio engine uninitialized successfully");
}

void AudioPlayer::load(const std::string &path) {
    DEBUG(std::format("Loading audio file: {}", path));
    unload(); // Für den Fall das was geladen war, das Freigeben

    if (audio_extractor.loadFile(path) == false) {
        throw std::runtime_error("Failed to load audio");
    }
    DEBUG("Audio file loaded successfully");

    if (!audio_extractor.getAudioData()) {
        is_something_loaded = false;
        return; // Keine Audiodaten, also nichts zu tun
    }
    DEBUG("Audio data available");

    // 2. Format für Miniaudio aus den Eigenschaften des Extractor
    constexpr ma_format format = ma_format_s16;   // wir haben AV_SAMPLE_FMT_S16 als Ziel
    const ma_uint32 channels = audio_extractor.getChannels();

    // 3. Audiodaten in den ma_audio_buffer kopieren
    const ma_audio_buffer_config config = ma_audio_buffer_config_init(
        format,
        channels,
        audio_extractor.getAudioDataSize() / (channels * ma_get_bytes_per_sample(format)), // Anzahl Frames
        audio_extractor.getAudioData(),
        nullptr   // keine eigene Allokationsfunktion
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

void AudioPlayer::play() {
    if (is_something_loaded) {
        DEBUG("Starting audio playback");
        ma_sound_start(&sound);
        playing = true;
        start_time = std::chrono::steady_clock::now();
        DEBUG("Playback started successfully");
    }
}

void AudioPlayer::stop() {
    if (is_something_loaded) {
        DEBUG("Stopping audio playback");
        ma_sound_stop(&sound);
        // Setzt Position nicht zurück, aber Wiedergabe pausiert
        playing = false;
        DEBUG("Playback stopped successfully");
    }
}

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

double AudioPlayer::get_time_ms() const {
    if (!playing) return 0.0;

    const auto now = std::chrono::steady_clock::now();
    DEBUG("Getting audio time: " + std::to_string(std::chrono::duration<double, std::milli>(now - start_time).count()));
    return std::chrono::duration<double, std::milli>(now - start_time).count();
}