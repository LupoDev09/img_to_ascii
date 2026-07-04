#include "Renderer.h"
#include "cxxopts.hpp"

#include <format>
#include <iostream>
#include <thread>

namespace fs = std::filesystem;


int main(const int argc, char** argv) {
    if (fs::exists("frames")) {
        fs::remove("frames");
    }
    fs::create_directory("frames");

    cxxopts::Options options("Img_to_ascii", "Img_to_ascii another rewrite");
    options.add_options()
    ("i,input", "Input video file", cxxopts::value<std::string>())
    ("w,width", "Width of the output ascii video", cxxopts::value<int>()->default_value("80"))
    ("h,height", "Height of the output ascii video", cxxopts::value<int>()->default_value("60"))
    ("f,frame-rate", "Frame rate of the output ascii video", cxxopts::value<int>()->default_value("30"))
    ("c,charset", "Charset to use for ascii mapping", cxxopts::value<std::string>()->default_value(" .:-=+*#%@"))
    ("help", "Print help");

    const cxxopts::ParseResult parse_result = options.parse(argc, argv);
    if (parse_result.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    const std::string input = parse_result["i"].as<std::string>();
    const int frame_rate = parse_result["f"].as<int>();
    const std::string charset = parse_result["c"].as<std::string>();
    const int width = parse_result["w"].as<int>();
    const int height = parse_result["h"].as<int>();

    const std::filesystem::path input_path = input;
    if (input.empty()) {
        std::cerr << "Input video file is required" << std::endl;
        return 1;
    }
    if (!fs::exists(input) || !fs::is_regular_file(input)) {
        std::cerr << "Input is invalid" << std::endl;
        return 1;
    }
    if (frame_rate <= 0) {
        std::cerr << "Frame rate is invalid" << std::endl;
        return 1;
    }
    if (height <= 0) {
        std::cerr << "Height is invalid" << std::endl;
        return 1;
    }
    if (width <= 0) {
        std::cerr << "Width is invalid" << std::endl;
        return 1;
    }

    std::cout << "Input video file: " << input << '\n';
    std::cout << "Frame rate: " << frame_rate << '\n';
    std::cout << "Charset: " << charset << '\n';
    std::cout << "Width: " << width << '\n';
    std::cout << "Height: " << height << '\n';

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

    if (fs::is_empty("frames")) {
        std::cerr << "No frames were extracted from the video. Check ffmpeg.log for details." << std::endl;
        return 1;
    }

    // Now the individual Frames should be in frames/
    Renderer renderer;
    Loader loader;
    renderer.config.color = true;
    if (!charset.empty()) {
        renderer.config.charset = charset;
    }

    std::vector<std::string> output_frames;
    while (true) {
        const Loader::Frame frame = loader.load_next_image();
        if (frame.data.empty()) { break; }
        output_frames.push_back(renderer.render_frame(frame));
    }

    // Print all Frames
    for (const std::string& frame : output_frames) {
        std::cout << frame << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / frame_rate));
        std::cout << "\033[2J\033[H" << std::flush; // Clear screen and move cursor to top-left
    }

    fs::remove_all("frames");
    return 0;
}
