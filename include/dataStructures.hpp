//
// Created by lupo on 04.07.26.
//

#ifndef IMG_TO_ASCII_DATASTRUCTURES_H
#define IMG_TO_ASCII_DATASTRUCTURES_H
#include <vector>

namespace DataStructures {
    /**
     * @struct Pixel
     * @brief Represents an RGB pixel with 8 bits per channel.
     */
    struct Pixel {
        uint8_t r;///< Red channel (0-255)
        uint8_t g;///< Green channel (0-255)
        uint8_t b;///< Blue channel (0-255)

        /**
         * @brief Calculate the perceived luminance of the pixel.
         * @return Luminance value using standard perception weights (BT.709).
         *
         * Uses the formula: L = 0.2126*R + 0.7152*G + 0.0722*B
         * This weighted average better matches human perception than simple averaging.
         */
        [[nodiscard]] uint8_t CalculateLuminance() const {
            // Uses BT.709 weights approximated with integer math:
            // L ≈ (0.2126 * R + 0.7152 * G + 0.0722 * B)
            // The integer form below computes (r*54 + g*183 + b*19) >> 8 which approximates the
            // floating point result while avoiding FP arithmetic for speed.
            return static_cast<uint8_t>((r * 54 + g * 183 + b * 19) >> 8);
        }
    };

    /**
     * @struct Frame
     * @brief Represents a decoded video frame with RGB pixel data.
     */
    struct Frame {
        int width{};            ///< Frame width in pixels
        int height{};           ///< Frame height in pixels
        double source_fps{};    ///< Original frame rate from the source video
        std::vector<Pixel> data;///< Pixel data in row-major order (width * height pixels)
    };
}// namespace DataStructures

#endif// IMG_TO_ASCII_DATASTRUCTURES_H
