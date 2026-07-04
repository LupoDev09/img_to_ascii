//
// Created by lupo on 04.07.26.
//

#ifndef IMG_TO_ASCII_RENDERER_H
#define IMG_TO_ASCII_RENDERER_H
#include "Data_structures/Frame.h"

class Renderer {
public:
    struct RenderConfig {
        int output_width{};
        int output_height{};
        bool use_color = true;
    };

    void render(const AsciiFrame& frame, const RenderConfig& config) {
        // TODO: implement
    };
};



#endif// IMG_TO_ASCII_RENDERER_H
