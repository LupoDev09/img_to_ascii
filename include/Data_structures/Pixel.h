//
// Created by lupo on 04.07.26.
//

#ifndef IMG_TO_ASCII_PIXEL_H
#define IMG_TO_ASCII_PIXEL_H
#include <cstdint>

struct Pixel {
    uint8_t r, g, b;

    /**
     * @brief Calculates the luminance of the pixel.
     * @return the calculated luminance
     */
    [[nodiscard]] float luminance() const {
        // Wahrnehmungsgewichtung (nicht einfach Mittelwert!)
        return 0.2126f * r + 0.7152f * g + 0.0722f * b;
    }
};

struct AsciiPixel {
    char symbol;
    Pixel color; // Originalfarbe behalten
};

#endif// IMG_TO_ASCII_PIXEL_H
