//
// Created by lupo on 04.07.26.
//

#include <Renderer.hpp>
#include <utf8_stuff.hpp>

inline char32_t Renderer::get_char(const float &luminance) const {
    const size_t charset_length = this->config.charset.length();
    const auto index = static_cast<size_t>(luminance / 255.0f * static_cast<float>(charset_length - 1));
    return this->config.charset.at(index);
}

std::string Renderer::render_frame(const DataStructures::Frame &frame) const {
    std::string output;
    output.reserve(frame.width * frame.height * 12);

    if (this->config.color) {
        for (int y = 0; y < frame.height; ++y) {
            for (int x = 0; x < frame.width; ++x) {
                const auto& pixel = frame.data[y * frame.width + x];

                output += "\033[38;2;";
                output += std::to_string(pixel.r);
                output += ";";
                output += std::to_string(pixel.g);
                output += ";";
                output += std::to_string(pixel.b);
                output += "m";

                output += utf_8_stuff::utf32_to_utf8(get_char(pixel.luminance()));

                output += "\033[0m";
            }
            output += '\n';
        }
    } else {
        for (int y = 0; y < frame.height; ++y) {
            for (int x = 0; x < frame.width; ++x) {
                const auto& pixel = frame.data[y * frame.width + x];
                output += utf_8_stuff::utf32_to_utf8(get_char(pixel.luminance()));
            }
            output += '\n';
        }
    }

    return output;
}
