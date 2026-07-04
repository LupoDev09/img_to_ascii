//
// Created by lupo on 04.07.26.
//

#ifndef IMG_TO_ASCII_LOADER_H
#define IMG_TO_ASCII_LOADER_H


#include <cstdint>
#include <vector>

class Loader {
    int current_frame = 1;

public:
    struct Pixel {
        uint8_t r, g, b;

        // Warum float? → bessere Genauigkeit bei Berechnung
        [[nodiscard]] float luminance() const {
            // Wahrnehmungsgewichtung (nicht einfach Mittelwert!)
            return 0.2126f * r + 0.7152f * g + 0.0722f * b;
        }
    };

    struct Frame {
        int width;
        int height;
        std::vector<Pixel> data;
    };

    /**
     * @brief Load the next image from the frames directory
     * @details Assumes that the frames are under a dir frames/ and are numerated in the format frame_000001.png
     * if the file does not exist, an empty Frame is returned
     * @return the next image as a pointer to unsigned char array
     */
    [[nodiscard]] Frame load_next_image();

};



#endif// IMG_TO_ASCII_LOADER_H
