#include <GenerateFrames.h>
#include <Renderer.h>
#include <cxxopts.hpp>

#include <dataStructures.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

bool VERBOSE_MODE = false;
#define VERBOSE(msg) if (VERBOSE_MODE) std::clog << msg << std::endl

int main(const int argc, char** argv) {
    cxxopts::Options options("Img_to_ascii", "Img_to_ascii another rewrite");
    options.add_options()
    ("i,input", "Input video file", cxxopts::value<std::string>())
    ("w,width", "Width of the output ascii video", cxxopts::value<int>()->default_value("80"))
    ("h,height", "Height of the output ascii video", cxxopts::value<int>()->default_value("60"))
    ("f,frame-rate", "Frame rate of the output ascii video", cxxopts::value<int>()->default_value("30"))
    ("use-source-fps", "Use the source video's frame rate", cxxopts::value<bool>()->default_value("false"))
    ("c,charset", "Charset to use for ascii mapping", cxxopts::value<std::string>()->default_value(" .:-=+*#%@"))
    ("v,verbose", "Enable verbose output", cxxopts::value<bool>()->default_value("false"))
    ("help", "Print help");

    const cxxopts::ParseResult parse_result = options.parse(argc, argv);
    if (parse_result.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    const std::string input = parse_result["i"].as<std::string>();
    const int frame_rate = parse_result["f"].as<int>();
    const std::string charset = parse_result["c"].as<std::string>();
    const bool verbose = parse_result["v"].as<bool>();
    const int width = parse_result["w"].as<int>();
    const int height = parse_result["h"].as<int>();
    const bool use_source_fps = parse_result["use-source-fps"].as<bool>();

    VERBOSE_MODE = verbose;

    const std::filesystem::path input_path = input;
    if (input.empty()) {
        std::cerr << "Input video file is required" << std::endl;
        return 1;
    }
    if (!fs::exists(input) || !fs::is_regular_file(input)) {
        std::cerr << "Input is invalid" << std::endl;
        return 1;
    }
    if (frame_rate <= 0 && !use_source_fps) {
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
    std::cout << "Use source fps: " << (use_source_fps ? "true" : "false") << '\n';
    std::cout << "Charset: " << charset << '\n';
    std::cout << "Width: " << width << '\n';
    std::cout << "Height: " << height << '\n';

    VERBOSE("Extracting frames from video...");
    const std::vector<DataStructures::Frame> frames = GenerateFrames::generate(input_path, frame_rate, width, height);
    VERBOSE("Finished extracting frames from video");

    if (frames.empty()) {
        std::cerr << "No frames were extracted from the video." << std::endl;
        return 1;
    }

    VERBOSE("Rendering frames to ascii...");
    Renderer renderer;
    renderer.config.color = true;
    if (!charset.empty()) {
        renderer.config.charset = charset;
    }

    // Keep decode and render separate: first build all frames in memory, then render them.
    const std::vector<std::string> rendered_frames = renderer.render_frames(frames);
    VERBOSE("Finished rendering frames to ascii");

    VERBOSE("Starting playback...");
    for (const std::string& frame : rendered_frames) {
        std::cout << frame << std::flush;
        if (use_source_fps) {
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(1000.0 / frames.front().source_fps)));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / frame_rate));
        }
        std::cout << "\033[2J\033[H" << std::flush;
    }
    VERBOSE("Finished playback");

    VERBOSE("Bye :3");
    return 0;
}
