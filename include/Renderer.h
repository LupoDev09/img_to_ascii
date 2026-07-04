//
// Created by lupo on 04.07.26.
//

#ifndef IMG_TO_ASCII_RENDERER_H
#define IMG_TO_ASCII_RENDERER_H
#include <string>
#include <vector>
#include <dataStructures.h>


class Renderer {
    [[nodiscard]] char get_char(const float &luminance) const;

public:
    struct Config {
        // Rendering uses the decoded frame dimensions, so only palette and color output are configurable here.
        bool color = true;
        std::string charset = " .:-=+*#%@";
    } config;

    // Convert a single RGB frame into one ANSI-colored ASCII string.
    [[nodiscard]] std::string render_frame(const DataStructures::Frame& frame) const;

    // Convert a batch of frames in parallel.
    [[nodiscard]] std::vector<std::string> render_frames(const std::vector<DataStructures::Frame>& frames) const;
};



#endif// IMG_TO_ASCII_RENDERER_H
