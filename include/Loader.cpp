//
// Created by lupo on 04.07.26.
//

#include "Loader.h"
#include <filesystem>
#include <format>
#include <stb_image_implementation.h>
namespace fs = std::filesystem;

Loader::Frame Loader::load_next_image() {
    const std::string current_frame_as_string = std::format("{:06d}", this->current_frame);
    const std::string path = fs::current_path().string() + "/frames/frame_" + current_frame_as_string + ".png";

    if (!fs::exists(path)) {
        return {};
    }

    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

    // Convert the loaded image data to the Frame structure
    Frame frame;
    frame.width = width;
    frame.height = height;
    frame.data.resize(width * height);

    for (int i = 0; i < width * height; ++i) {
        frame.data.at(i) = {data[i * channels], data[i * channels + 1], data[i * channels + 2]};
    }

    this->current_frame += 1; // Increment the frame counter for the next call
    stbi_image_free(data);
    return frame;
}