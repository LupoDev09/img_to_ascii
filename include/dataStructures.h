//
// Created by lupo on 04.07.26.
//

#ifndef IMG_TO_ASCII_DATASTRUCTURES_H
#define IMG_TO_ASCII_DATASTRUCTURES_H
#include <cstdint>
#include <vector>

namespace DataStructures {
    struct Pixel {
        uint8_t r, g, b;

        [[nodiscard]] float luminance() const {
            // Wahrnehmungsgewichtung (nicht einfach Mittelwert!)
            return 0.2126f * static_cast<float>(r)
                 + 0.7152f * static_cast<float>(g)
                 + 0.0722f * static_cast<float>(b);
        }
    };

    struct Frame {
        int width;
        int height;
        double source_fps;
        std::vector<Pixel> data;
    };
}

#endif// IMG_TO_ASCII_DATASTRUCTURES_H
