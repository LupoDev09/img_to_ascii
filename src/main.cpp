#include <GenerateFrames.h>
#include <Renderer.h>
#include <cxxopts.hpp>

#include <dataStructures.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

bool VERBOSE_MODE = false;
#define VERBOSE(msg) if (VERBOSE_MODE) std::clog << msg << std::endl

/**
 * @brief Outputs the rendered frames to the console with a specified frame rate.
 * @param rendered_frames the frames to print to the screen
 * @param source_fps the source_fps
 * @param frame_rate the set fps
 */
void outputFrames(std::deque<std::string> &rendered_frames, const double source_fps, const int frame_rate) {
    VERBOSE("Starting playback...");
    if (rendered_frames.size() == 1)
        std::cout << rendered_frames.back() << std::flush;
    else {
        size_t frame_count = rendered_frames.size();
        const double target_ms = frame_rate > 0 ? (1000.0 / frame_rate) : (1000.0 / source_fps);
        while (!rendered_frames.empty()) {
            std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

            std::cout << rendered_frames.back() << std::flush;
            rendered_frames.pop_back(); // Remove the rendered frame
            frame_count--; // decrement frame counter

            // Calc the time to sleep between frames
            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
            const double elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
            const double sleep_ms = std::max(0.0, target_ms - elapsed_ms);
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(sleep_ms)));

            if (frame_count > 0) {
                std::cout << "\033[2J\033[H" << std::flush;
            }
        }
    }
    VERBOSE("Finished playback");
}

/*
 * TODO: Implement Audio support
 * TODO: Try to Multy-thread the Frame Generation
 * TODO: Try to minimize RAM usage
 */
int main(const int argc, char** argv) {
    cxxopts::Options options("Img_to_ascii", "Img_to_ascii another rewrite");

    // Group: Input
    options.add_options("Input")
    ("i,input", "Input video file", cxxopts::value<std::string>());

    // Group: Sizing (width / height)
    options.add_options("Sizing")
    ("w,width", "Width of the output ascii video", cxxopts::value<int>()->default_value("0"))
    ("h,height", "Height of the output ascii video", cxxopts::value<int>()->default_value("0"));

    // Group: Video / Playback
    options.add_options("Playback")
    ("f,fps", "Frame rate of the output ascii video (default is source)", cxxopts::value<int>()->default_value("0"));

    // Group: Output
    options.add_options("Output")
    ("no-color", "Disable the color in the output", cxxopts::value<bool>()->default_value("false"))
    ("c,charset", "Charset to use for ascii mapping", cxxopts::value<std::string>()->default_value(" .:-=+*#%@"));

    // Group: General
    options.add_options("General")
    ("v,verbose", "Enable verbose output", cxxopts::value<bool>()->default_value("false"))
    ("help", "Print help")
    ("no-output", "Do not output the ascii video to stdout", cxxopts::value<bool>()->default_value("false"));

    const cxxopts::ParseResult parse_result = options.parse(argc, argv);
    if (parse_result.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    if (!parse_result.count("i")) {
        std::cerr << "Missing required option: --input\n";
        std::cout << options.help() << std::endl;
        return 1;
    }

    const std::string input = parse_result["i"].as<std::string>();
    const int frame_rate = parse_result["f"].as<int>();
    const std::string charset = parse_result["c"].as<std::string>();
    const bool verbose = parse_result["v"].as<bool>();
    const int width = parse_result["w"].as<int>();
    const int height = parse_result["h"].as<int>();
    const bool no_output = parse_result["no-output"].as<bool>();
    const bool no_color = parse_result["no-color"].as<bool>();

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
    if (frame_rate < 0) {
        std::cerr << "Frame rate is invalid" << std::endl;
        return 1;
    }
    if (height <= 0 && width <= 0) {
        std::cerr << "Height and width are invalid please provide at least one of them as positiv integer" << std::endl;
        return 1;
    }

    std::cout << "Input video file: " << input << '\n';
    std::cout << "Frame rate: " << (frame_rate > 0 ? std::to_string(frame_rate) : "use source") << '\n';
    std::cout << "Charset: " << charset << '\n';
    std::cout << "Width: " << width << '\n';
    std::cout << "Height: " << height << '\n';
    std::cout << "Verbose: " << (verbose ? "true" : "false") << '\n';
    std::cout << "No Color: " << (no_color ? "true" : "false") << '\n';
    std::cout << "No Output: " << (no_output ? "true" : "false") << '\n';
    std::cout << "Starting video to ascii conversion..." << std::endl;

    VERBOSE("Extracting frames from video...");
    auto frames = std::make_unique<std::vector<DataStructures::Frame>>();
    try {
        GenerateFrames::generate(*frames, input_path, frame_rate, width, height);
    } catch (std::runtime_error &e) {
        std::cerr << "Error extracting frames: " << e.what() << std::endl;
        return 1;
    } catch (std::invalid_argument &e) {
        std::cerr << "Invalid argument: " << e.what() << std::endl;
        return 1;
    }

    VERBOSE("Finished extracting frames from video");

    if (frames->empty()) {
        std::cerr << "No frames were extracted from the video." << std::endl;
        return 1;
    }

    VERBOSE("Rendering frames to ascii...");
    Renderer renderer;
    renderer.config.color = !no_color;
    if (!charset.empty()) {
        renderer.config.charset = charset;
    }

    // Keep decode and render separate: first build all frames in memory, then render them.
    std::deque<std::string> rendered_frames = renderer.render_frames(*frames); // The Frames are in reverse order
    double source_fps = frames->front().source_fps;
    frames.reset(); // Free memory used by decoded frames, we don't need them anymore.
    VERBOSE("Finished rendering frames to ascii");

    if (!no_output) {
        outputFrames(rendered_frames, source_fps, frame_rate);
    } else {
        VERBOSE("Output to stdout is disabled, skipping playback");
    }

    VERBOSE("Bye :3");
    return 0;
}
