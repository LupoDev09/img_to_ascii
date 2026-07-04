//
// Created by lupo on 04.07.26.
//

#include "../include/Renderer.h"

#include <format>

char Renderer::get_char(const float &luminance) const {
    const size_t charset_length = this->config.charset.length();
    const auto index = static_cast<size_t>(luminance / 255.0f * (charset_length - 1));
    return this->config.charset.at(index);
}

std::string Renderer::render_frame(const Loader::Frame& frame) const {
    std::string output;
    if (this->config.color) {
        for (int y = 0; y < frame.height; ++y) {
            for (int x = 0; x < frame.width; ++x) {
                const Loader::Pixel& pixel = frame.data.at(y * frame.width + x);
                // 24-bit Truecolor: \033[38;2;<r>;<g>;<b>m
                output += std::format("\033[38;2;{};{};{}m{}\033[0m", pixel.r, pixel.g, pixel.b, get_char(pixel.luminance()));
            }
            output += '\n';
        }
    } else {
        for (int y = 0; y < frame.height; ++y) {
            for (int x = 0; x < frame.width; ++x) {
                const Loader::Pixel& pixel = frame.data.at(y * frame.width + x);
                output += get_char(pixel.luminance());
            }
            output += '\n';
        }
    }
    return output;
}
