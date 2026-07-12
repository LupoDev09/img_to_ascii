#include <AudioPlayer.hpp>
#include <SyncClock.hpp>
#include <utf8_stuff.hpp>
#include <Verbose.hpp>

#include <GenerateFrames.hpp>
#include <Renderer.hpp>
#include <cxxopts.hpp>
#include <dataStructures.hpp>

#include <chrono>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;


/**
 * @brief Convert a video file to ASCII art animation.
 * 
 * Reads a video file, extracts frames and audio, scales frames to specified dimensions,
 * converts pixels to ASCII characters based on luminance, and displays the animation
 * in the terminal with optional color support and synchronized audio.
 * 
 * Workflow:
 * 1. Parse command-line arguments for input file, output dimensions, frame rate, etc.
 * 2. Validate input parameters (file exists, dimensions valid, etc.)
 * 3. Extract and prepare audio from video file
 * 4. Configure renderer with color and character palette settings
 * 5. Decode video frames in a loop:
 *    - Scale frames to target dimensions
 *    - Convert each frame to ASCII art
 *    - Output ASCII art with proper timing
 *    - Synchronize with audio playback
 * 6. Clean up resources (audio file, etc.)
 * 
 * @return 0 on success, 1 on error
 */
int main(const int argc, char** argv) {
    cxxopts::Options options("Img_to_ascii", "Convert video files to ASCII art animations");

    // Group: Input
    options.add_options("Input")
    ("i,input", "Input video file", cxxopts::value<std::string>());

    // Group: Sizing (width / height)
    options.add_options("Sizing")
    ("w,width", "Width of the output ascii video (0 = auto-calculate from height)", cxxopts::value<int>()->default_value("0"))
    ("h,height", "Height of the output ascii video (0 = auto-calculate from width)", cxxopts::value<int>()->default_value("0"));

    // Group: Video / Playback
    options.add_options("Playback")
    ("f,fps", "Frame rate of the output ascii video (0 = use source frame rate)", cxxopts::value<int>()->default_value("0"))
    ("no-audio", "Disable audio playback during animation", cxxopts::value<bool>()->default_value("false"));

    // Group: Output
    options.add_options("Output")
    ("no-color", "Disable ANSI color output (output grayscale only)", cxxopts::value<bool>()->default_value("false"))
    ("c,charset", "Character palette for ASCII mapping (darker to lighter)", cxxopts::value<std::string>()->default_value(" ░▒▓█"));

    // Group: General
    options.add_options("General")
    ("help", "Print this help message")
    ("no-output", "Process video without outputting ASCII animation to stdout", cxxopts::value<bool>()->default_value("false"));

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
    const std::string charset = parse_result["c"].as<std::string>();
    const int width = parse_result["w"].as<int>();
    const int height = parse_result["h"].as<int>();
    const int frame_rate = parse_result["f"].as<int>();
    const bool no_output = parse_result["no-output"].as<bool>();
    const bool no_color = parse_result["no-color"].as<bool>();
    const bool no_audio = parse_result["no-audio"].as<bool>();
    std::u32string charset32 = utf_8_stuff::utf8_to_utf32(charset);

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

    std::cout << "Input video file: " << input << '\n'
              << "Frame rate: " << (frame_rate > 0 ? std::to_string(frame_rate) : "use source") << '\n'
              << "Charset: " << charset << '\n'
              << "Width: " << width << '\n'
              << "Height: " << height << '\n'
              << "No Color: " << (no_color ? "true" : "false") << '\n'
              << "No Output: " << (no_output ? "true" : "false") << '\n'
              << "No Audio: " << (no_audio ? "true" : "false") << '\n'
              << "Starting video to ascii conversion..." << std::endl;

    AudioPlayer audio;
    SyncClock clock;

    std::string audio_file = "audio.wav";

    DEBUG("Loading audio file...");
    if (no_audio) {
        DEBUG("Audio playback is disabled.");
    } else {
        DEBUG("Audio playback is enabled.");
        audio.load(input);
        DEBUG("Audio file loaded successfully.");
    }

    DEBUG("Configure Renderer");
    Renderer renderer;
    renderer.config.color = !no_color;
    if (!charset32.empty()) {
        renderer.config.charset = charset32;
    }
    DEBUG("Configured Renderer");

    DEBUG("Starting frame generation and output...");
    bool first_frame = true;
    double target_ms = 0.0;
    unsigned int frame_index = 0;
    GenerateFrames::generate(input_path, frame_rate, width, height,
        [&](DataStructures::Frame&& frame) {
            // Clear previous frame and move cursor to home position
            if (!first_frame && !no_output) {
                std::cout << "\033[2J\033[H";
            }

            // Initialize timing and playback on first frame
            if (first_frame) {
                clock.start();
                if (!no_output && !no_audio) {
                    audio.play();
                }
                const double source_fps = frame.source_fps;

                // Calculate target time per frame (either custom fps or source fps)
                target_ms = frame_rate > 0
                ? 1000.0 / frame_rate
                : 1000.0 / source_fps;

                first_frame = false;
            }

            // Convert frame to ASCII art
            const std::string rendered = renderer.render_frame(frame);

            if (!no_output) {
                std::cout << rendered << std::flush;
            }

            // Synchronize with calculated frame time
            const double expected = frame_index * target_ms;
            clock.wait_until(expected);
            frame_index++;
    });

    DEBUG("Frame generation and output completed.");

    DEBUG("Cleaning up audio resources...");
    audio.stop();
    audio.unload();
    DEBUG("Audio file deleted");

    std::cout << "Bye :3" << std::endl;
    return 0;
}
