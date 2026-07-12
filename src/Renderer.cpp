//
// Created by lupo on 04.07.26.
//

#include <Renderer.hpp>

#include "Verbose.hpp"

#include <format>
#include <utf8_stuff.hpp>

void Renderer::build_char_lut() {
    const auto& charset = config.charset;
    const size_t n = charset.size();

    for (size_t i = 0; i < 256; ++i) {
        const auto index = static_cast<size_t>(static_cast<float>(i) / 255.0f * static_cast<float>(n - 1));

        char_lut[i] = utf_8_stuff::utf32_to_utf8(charset[index]);
    }
}

Renderer::Renderer() {
    // Generiert einen Lookup table für Zahlen, als strings um die nicht immer während des rendering zu generieren
    for (int i = 0; i < 256; ++i) {
        number_lut[i] = std::to_string(i);
    }
    build_char_lut();
}

void Renderer::set_charset(const std::u32string &charset) {
    this->config.charset = charset;
    // Rebuild the character lookup table
    this->build_char_lut();
}

std::string Renderer::render_frame(const DataStructures::Frame &frame) const {
    DEBUG("Renderer: render_frame got called");
    DEBUG(std::format("Rendering frame of size {}x{}", frame.width, frame.height));
    std::string output;

    if (this->config.color) {
        DEBUG("Rendering with color enabled");
        output.append(COLOR_RESET);

        uint8_t last_r = 0, last_g = 0, last_b = 0;

        // 25 ist die ungefähre anzahl in bytes die ich pro pixel brauche
        output.reserve(frame.width * frame.height * 26);
        for (int y = 0; y < frame.height; ++y) {
            bool first_pixel_in_line = true;
            const DataStructures::Pixel * row = &frame.data[y * frame.width];
#ifdef DEBUG_RENDERER_MODE
            output.append(std::to_string(y));
#endif

            for (int x = 0; x < frame.width; ++x) {
                const auto& pixel = row[x];

                // Änder die Ansi sequence nur, wenn sie anders ist als die vorherige oder es der erste frame pixel ist
                if (first_pixel_in_line || pixel.r != last_r || pixel.g != last_g || pixel.b != last_b) {
                    output.append(COLOR_PREFIX);
                    output.append(number_lut[pixel.r]);
                    output.push_back(';');
                    output.append(number_lut[pixel.g]);
                    output.push_back(';');
                    output.append(number_lut[pixel.b]);
                    output.push_back('m');

                    last_r = pixel.r;
                    last_g = pixel.g;
                    last_b = pixel.b;
                    first_pixel_in_line = false;
                }

                output.append(char_lut[pixel.CalculateLuminance()]);
            }
            output.append(COLOR_RESET);  // Reset color at the end of each line
            output.push_back('\n');
        }
    } else {
        DEBUG("Rendering with color disabled");
        output.reserve(frame.width * frame.height * 12);
        for (int y = 0; y < frame.height; ++y) {
            const DataStructures::Pixel * row = &frame.data[y * frame.width];
            for (int x = 0; x < frame.width; ++x) {
                const auto& pixel = row[x];
                output.append(char_lut[pixel.CalculateLuminance()]);
            }
            output.push_back('\n');
        }
    }

    return output;
}
