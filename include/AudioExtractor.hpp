//
// Created by lupo on 12.07.26.
//

#ifndef IMG_TO_ASCII_AUDIOEXTRACTOR_HPP
#define IMG_TO_ASCII_AUDIOEXTRACTOR_HPP

#include <memory>
#include <string>
#include <vector>

extern "C" {
#include <libswresample/swresample.h>
}

class AudioExtractor {
public:
    /**
     * @brief Constructs an AudioExtractor and optionally loads an audio file.
     * @param filepath the file to load (optional)
     * @throws std::runtime_error if something goes wrong in the extraction process
     */
    explicit AudioExtractor(const std::string& filepath = "");

    /**
     * @brief Destroys the AudioExtractor and frees its resources.
     */
    ~AudioExtractor();

    // Keine Kopien oder Zuweisungen
    AudioExtractor(const AudioExtractor&) = delete;
    AudioExtractor& operator=(const AudioExtractor&) = delete;

    /**
     * @brief Loads an audio file and extracts its audio stream into memory.
     * @param filepath the path to the file to load
     * @return whether the extraction was successful
     * @throws std::runtime_error if the extraction fails
     */
    [[nodiscard]] bool loadFile(const std::string& filepath);

    /**
     * @brief Checks if the extractor has successfully extracted an audio stream from the loaded file.
     * @return whether the extractor has successfully extracted an audio stream from the loaded file
     */
    [[nodiscard]] bool hasAudio() const;

    /**
     * @brief Returns a pointer to the extracted audio data.
     * @return a pointer to the extracted audio data (nullptr if no audio is available)
     */
    [[nodiscard]] const uint8_t* getAudioData() const;

    /**
     * @brief Frees the memory allocated for the extracted audio data.
     * @return whether the operation was successful
     */
    [[nodiscard]] bool freeAudioData();

    /**
     * @brief Returns the size of the extracted audio data in bytes.
     * @return the size of the extracted audio data in bytes
     */
    [[nodiscard]] size_t getAudioDataSize() const;

    /**
     * @brief Returns the sample rate of the extracted audio data.
     * @return the sample rate of the extracted audio data
     */
    [[nodiscard]] int getSampleRate() const;

    /**
     * @brief Returns the number of channels in the extracted audio data.
     * @return the number of channels in the extracted audio data
     */
    [[nodiscard]] int getChannels() const;

    /**
     * @brief Returns the sample format of the extracted audio data.
     * @return the sample format of the extracted audio data
     */
    [[nodiscard]] AVSampleFormat getSampleFormat() const;

private:
    std::vector<uint8_t> audioBuffer;
    int sampleRate = 0;
    int channels = 0;
    AVSampleFormat sampleFmt = AV_SAMPLE_FMT_NONE;
    bool audioFound = false;

    /**
     * @brief Extracts the audio stream from the given video file and stores it in memory.
     * @param filepath the path to the video file from which to extract audio
     * @throws std::runtime_error if the extraction fails
     */
    void extractAudio(const std::string& filepath);
};


#endif// IMG_TO_ASCII_AUDIOEXTRACTOR_HPP
