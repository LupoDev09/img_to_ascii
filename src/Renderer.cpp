//
// Created by lupo on 04.07.26.
//

#include "../include/Renderer.h"

#include <atomic>
#include <format>
#include <thread>

namespace std {
    class thread;
}
char Renderer::get_char(const float &luminance) const {
    const size_t charset_length = this->config.charset.length();
    const auto index = static_cast<size_t>(luminance / 255.0f * static_cast<float>(charset_length - 1));
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

std::vector<std::string> Renderer::render_frames() const {
    Loader loader;
    std::vector<Loader::Frame> frames;
    while (true) {
        const Loader::Frame frame = loader.load_next_image();
        if (frame.data.empty()) {
            break;
        }
        frames.push_back(frame);
    }

    std::vector<std::string> output_frames(frames.size());
    const std::size_t worker_count = std::max<std::size_t>(
        1,
        std::min<std::size_t>(
            frames.size(),
            std::thread::hardware_concurrency() == 0 ? 1 : std::thread::hardware_concurrency()
        )
    );

    std::atomic_size_t next_frame_index{0};
    std::vector<std::thread> workers;
    workers.reserve(worker_count);
    for (std::size_t i = 0; i < worker_count; ++i) {
        workers.emplace_back([&]() {
            while (true) {
                const std::size_t index = next_frame_index.fetch_add(1, std::memory_order_relaxed);
                if (index >= frames.size()) {
                    break;
                }
                output_frames[index] = render_frame(frames[index]);
            }
        });
    }

    for (std::thread& worker : workers) {
        worker.join();
    }

    return output_frames;
}
