//
// Created by lupo on 04.07.26.
//

#ifndef IMG_TO_ASCII_RENDERER_H
#define IMG_TO_ASCII_RENDERER_H
#include <dataStructures.h>
#include <string>


class Renderer {
    [[nodiscard]] char get_char(const float &luminance) const;

public:
    struct Config {
        // Rendering uses the decoded frame dimensions, so only palette and color output are configurable here.
        bool color = true;
        std::string charset = " .:-=+*#%@";
    } config;

    /**
     * @brief Render a frame into a string representation using the current configuration.
     * @param frame the frame to process
     * @return the rendered frame
     */
    [[nodiscard]] std::string render_frame(const DataStructures::Frame& frame) const;
};



#endif// IMG_TO_ASCII_RENDERER_H
