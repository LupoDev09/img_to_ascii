//
// Created by lupo on 04.07.26.
//

#ifndef IMG_TO_ASCII_RENDERER_H
#define IMG_TO_ASCII_RENDERER_H
#include <string>
#include <Loader.h>


class Renderer {
    [[nodiscard]] char get_char(const float &luminance) const;

public:
    struct Config {
        // In Normal cases height and width should be configurable with this struct but
        // because I use ffmpeg for the extracting this is not needed
        bool color = true;
        std::string charset = " .:-=+*#%@";
    } config;

    [[nodiscard]] std::string render_frame(const Loader::Frame& frame) const;

    [[nodiscard]] std::vector<std::string> render_frames() const;
};



#endif// IMG_TO_ASCII_RENDERER_H
