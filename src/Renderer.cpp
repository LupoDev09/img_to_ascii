//
// Created by lupo on 04.07.26.
//

#include <Renderer.hpp>

#include "Verbose.hpp"

#include <format>
#include <utf8.h>

void Renderer::build_char_lut() {
    DEBUG("Building character lookup table");
    const auto& charset = config.charset;
    const size_t n = charset.size();

    for (size_t i = 0; i < 256; ++i) {
        const auto index = static_cast<size_t>(static_cast<float>(i) / 255.0f * static_cast<float>(n - 1));
        utf8::utf32to8(&charset[index], &charset[index + 1], std::back_inserter(m_char_lut[i]));
    }
}

void Renderer::build_padding() {
    DEBUG("Build padding got called");
    if (int pad = config.left_pad; pad > 0) {
        DEBUG("Left padding is set");
        for (; pad > 0; --pad) {
            m_left_pad_str.push_back(' ');
        }
    } else {
        DEBUG("Left padding is disabled");
        m_left_pad_str.clear();
    }
}

Renderer::Renderer() {
    DEBUG("Initializing Renderer");
    // Generiert einen Lookup table für Zahlen, als strings um die nicht immer während des rendering zu generieren
    for (int i = 0; i < 256; ++i) {
        m_number_lut[i] = std::to_string(i);
    }
    build_char_lut();
}
Renderer::~Renderer() {
    DEBUG("Destroying Renderer");
}

void Renderer::set_charset(const std::u32string &charset) {
    DEBUG("set_charset got called");
    this->config.charset = charset;
    // Rebuild the character lookup table
    this->build_char_lut();
}

void Renderer::set_left_pad(const int left_pad) {
    DEBUG("set_left_pad got called with pad: " + std::to_string(left_pad));
    this->config.left_pad = left_pad;
    build_padding();
}

std::string Renderer::render_frame(const DataStructures::Frame &frame) const {
    DEBUG("Renderer: render_frame got called");
    DEBUG(std::format("Rendering frame of size {}x{}", frame.width, frame.height));
    std::string output;

    if (this->config.color) {
        DEBUG("Rendering with color enabled");
        output.append(COLOR_RESET);

        uint8_t last_r = 0, last_g = 0, last_b = 0;

        // 25 ist die ungefähre anzahl in bytes die ich pro pixel brauche + 1 zur sicher heit
        output.reserve(static_cast<size_t>(frame.width * frame.height * 26 + m_left_pad_str.size() * frame.height));
        for (int y = 0; y < frame.height; ++y) {
            bool first_pixel_in_line = true;
            const DataStructures::Pixel * row = &frame.data[y * frame.width];
#ifdef DEBUG_RENDERER_MODE
            output.append(std::to_string(y));
#endif

            output.append(m_left_pad_str);
            for (int x = 0; x < frame.width; ++x) {
                const auto& pixel = row[x];

                // Änder die Ansi sequence nur, wenn sie anders ist als die vorherige oder es der erste frame pixel ist
                if (first_pixel_in_line || pixel.r != last_r || pixel.g != last_g || pixel.b != last_b) {
                    output.append(COLOR_PREFIX);
                    output.append(m_number_lut[pixel.r]);
                    output.push_back(';');
                    output.append(m_number_lut[pixel.g]);
                    output.push_back(';');
                    output.append(m_number_lut[pixel.b]);
                    output.push_back('m');

                    last_r = pixel.r;
                    last_g = pixel.g;
                    last_b = pixel.b;
                    first_pixel_in_line = false;
                }

                output.append(m_char_lut[pixel.CalculateLuminance()]);
            }
            output.append(COLOR_RESET);  // Reset color at the end of each line
            output.push_back('\n');
        }
    } else {
        DEBUG("Rendering with color disabled");
        output.reserve(static_cast<size_t>(frame.width * frame.height * 12 + m_left_pad_str.size() * frame.height));
        for (int y = 0; y < frame.height; ++y) {
            const DataStructures::Pixel * row = &frame.data[y * frame.width];
            output.append(m_left_pad_str);
            for (int x = 0; x < frame.width; ++x) {
                const auto& pixel = row[x];
                output.append(m_char_lut[pixel.CalculateLuminance()]);
            }
            output.push_back('\n');
        }
    }

    return output;
}
