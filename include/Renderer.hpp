//
// Created by lupo on 04.07.26.
//

#ifndef IMG_TO_ASCII_RENDERER_H
#define IMG_TO_ASCII_RENDERER_H
#include <dataStructures.h>
#include <string>

/**
 * @class Renderer
 * @brief Converts video frames into ASCII art with optional color support.
 * 
 * The Renderer maps pixel luminance values to characters from a configurable charset,
 * creating terminal-displayable ASCII art. It can output with ANSI color codes for
 * colored terminals or plain text for standard terminals.
 */
class Renderer {
    /**
     * @brief Map a luminance value to a character from the charset.
     * @param luminance Perceived brightness (0.0 - 255.0)
     * @return Character corresponding to the luminance level
     * 
     * Maps luminance linearly to charset indices, where darker characters represent
     * lower luminance and brighter characters represent higher luminance.
     */
    [[nodiscard]] char32_t get_char(const float &luminance) const;

public:
    /**
     * @struct Config
     * @brief Rendering configuration options.
     */
    struct Config {
        /// Enable ANSI color codes in output (true = colored, false = grayscale)
        bool color = true;
        /// Character palette for luminance mapping (darker to lighter characters)
        std::u32string charset = U" ░▒▓█";
    } config;

    /**
     * @brief Render a video frame into ASCII art.
     * 
     * Converts pixel data to ASCII characters based on luminance values.
     * Output includes ANSI terminal codes for positioning and optionally coloring.
     * 
     * @param frame The decoded video frame to render
     * @return String containing ANSI-formatted ASCII art with embedded control codes
     */
    [[nodiscard]] std::string render_frame(const DataStructures::Frame &frame) const;
};

#endif// IMG_TO_ASCII_RENDERER_H
