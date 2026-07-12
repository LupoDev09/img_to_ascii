//
// Created by lupo on 12.07.26.
//

#ifndef IMG_TO_ASCII_AUDIOEXTRACTOR_HPP
#define IMG_TO_ASCII_AUDIOEXTRACTOR_HPP

#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include <cstring>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

class AudioExtractor {
public:
    // Konstruktor: extrahiert die Audiospur aus der übergebenen Datei
    explicit AudioExtractor(const std::string& filepath = "");

    // Destruktor
    ~AudioExtractor();

    // Keine Kopien oder Zuweisungen
    AudioExtractor(const AudioExtractor&) = delete;
    AudioExtractor& operator=(const AudioExtractor&) = delete;

    // Lädt die Audiospur aus der angegebenen Datei und speichert sie intern
    [[nodiscard]] bool loadFile(const std::string& filepath);

    // Liefert true, wenn eine Audiospur gefunden wurde
    [[nodiscard]] bool hasAudio() const;

    // Zeiger auf die extrahierten Audiodaten (nullptr, falls keine Spur vorhanden)
    [[nodiscard]] const uint8_t* getAudioData() const;
    [[nodiscard]] bool freeAudioData();

    // Größe der Audiodaten in Bytes
    [[nodiscard]] size_t getAudioDataSize() const;

    // Eigenschaften des extrahierten Audios
    [[nodiscard]] int getSampleRate() const;
    [[nodiscard]] int getChannels() const;
    [[nodiscard]] AVSampleFormat getSampleFormat() const;

private:
    std::vector<uint8_t> audioBuffer;
    int sampleRate    = 0;
    int channels      = 0;
    AVSampleFormat sampleFmt = AV_SAMPLE_FMT_NONE;
    bool audioFound   = false;

    // Extraktionslogik
    void extractAudio(const std::string& filepath);
};


#endif// IMG_TO_ASCII_AUDIOEXTRACTOR_HPP
