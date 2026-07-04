//
// Created by lupo on 04.07.26.
//

#include <GenerateFrames.h>

#include <filesystem>
#include <format>
#include <string>

void GenerateFrames::generate(const std::filesystem::path & input_path, const int frame_rate, const int width, const int height) {
    const std::string command = std::format(
        "ffmpeg -y -i \"{}\" "
        "-vf \"fps={},scale={}:{}:flags=bilinear\" "
        "frames/frame_%06d.png > ffmpeg.log 2>&1",
        input_path.string(),
        frame_rate,
        width,
        height
    );
    std::system(command.c_str());
}