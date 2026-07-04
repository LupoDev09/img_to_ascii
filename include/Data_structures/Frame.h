//
// Created by lupo on 04.07.26.
//

#ifndef IMG_TO_ASCII_FRAME_H
#define IMG_TO_ASCII_FRAME_H
#include <Data_structures/Pixel.h>
#include <vector>

struct Frame {
    int width;
    int height;
    std::vector<Pixel> data;

    [[nodiscard]] Pixel& at(const int x, const int y) {
        // row-major layout → cache friendly
        return data[y * width + x];
    }

    [[nodiscard]] const Pixel& at(const int x, const int y) const {
        return data[y * width + x];
    }
};

struct AsciiFrame {
    int width;
    int height;
    std::vector<AsciiPixel> data;
};

#endif// IMG_TO_ASCII_FRAME_H
