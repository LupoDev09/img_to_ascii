//
// Created by lupo on 04.07.26.
//

#ifndef IMG_TO_ASCII_RENDERER_H
#define IMG_TO_ASCII_RENDERER_H
#include <array>
#include <dataStructures.hpp>
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
public:
    Renderer();
    ~Renderer();

    // Keine Kopien oder Zuweisungen
    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;
    Renderer(Renderer &&) = delete;
    Renderer &operator=(Renderer &&) = delete;


    /**
     * @struct Config
     * @brief Rendering configuration options.
     */
    struct Config {
        /// Enable ANSI color codes in output (true = colored, false = grayscale)
        bool color = true;

        /// Character palette for luminance mapping (darker to lighter characters)
        std::u32string charset = U" ░▒▓█";

        /// Left padding for each line of ASCII art
        int left_pad = 0;

    } config;

    void set_charset(const std::u32string &charset);

    void set_left_pad(int left_pad);

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
private:
    std::array<std::string, 256> m_number_lut;  ///< Lookup table for luminance to character mapping
    std::array<std::string, 256> m_char_lut;   ///< Lookup table for character mapping

    std::string m_left_pad_str;

    static constexpr std::string_view COLOR_PREFIX = "\033[38;2;";
    static constexpr std::string_view COLOR_RESET = "\033[0m";

    void build_char_lut();

    void build_padding();
};

#endif// IMG_TO_ASCII_RENDERER_H
