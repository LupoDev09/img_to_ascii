/*
 * @file main.cpp
 * @brief Command-line front-end: parse arguments and orchestrate extraction, rendering and playback.
 *
 * Main responsibilities:
 * - Parse CLI options and validate inputs
 * - Initialize AudioPlayer and Renderer according to flags
 * - Decode frames and hand them to the Renderer for display
 * - Coordinate teardown and resource cleanup
 */
#define MINIAUDIO_IMPLEMENTATION
// ReSharper disable once CppUnusedIncludeDirective
#include <miniaudio.h>

#include <AudioPlayer.hpp>
#include <OutputWriter.hpp>
#include <Renderer.hpp>
#include <Verbose.hpp>
#include <chrono>
#include <cxxopts.hpp>
#include <dataStructures.hpp>
#include <filesystem>
#include <iostream>
#include <utf8/checked.h>
#include "RawOutputGuard.hpp"

namespace {
    // Makes the cursor invisible on construction and visible through a function call or at deconstruction
    struct CursorGuard {
        CursorGuard() {
            std::cout << "\033[?25l";// Hide cursor
        }

        static void makeVisible() {
            std::cout << "\033[?25h";// Show cursor
        }

        ~CursorGuard() { makeVisible(); }
    };

    struct FrameHandler {
        Renderer& renderer;

        void operator()(DataStructures::Frame&& frame) const {
            // add_decoded_frame is blocking and provides backpressure, so a simple call is sufficient
            renderer.add_decoded_frame(std::move(frame));
        }
    };
}// namespace

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
int main(int argc, char** argv) {
    cxxopts::Options options("Img_to_ascii", "Convert video files to ASCII art animations");

    // Group: Input
    options.add_options("Input")("i,input", "Input video file", cxxopts::value<std::string>());

    // Group: Sizing (width / height)
    options.add_options("Sizing")("w,width", "Width of the output ascii video (0 = auto-calculate from height)",
            cxxopts::value<int>()->default_value("0"))("h,height",
            "Height of the output ascii video (0 = auto-calculate from width)",
            cxxopts::value<int>()->default_value("0"));

    // Group: Video / Playback
    options.add_options("Playback")
    ("f,fps", "Frame rate of the output ascii video (0 = use source frame rate)",
            cxxopts::value<int>()->default_value("0"))
    ("no-audio", "Disable audio playback during animation",
            cxxopts::value<bool>()->default_value("false"));

    // Group: Output
    options.add_options("Output")("no-color", "Disable ANSI color output (output grayscale only)",
            cxxopts::value<bool>()->default_value("false"))("c,charset",
            "Character palette for ASCII mapping (darker to lighter) max 256 characters",
            cxxopts::value<std::string>()->default_value(" ░▒▓█"))("left-pad",
            "Left padding for each line of ASCII art", cxxopts::value<int>()->default_value("0"))
            ("o, output-file", "Output file for ASCII animation (if not specified, output to stdout)",
                    cxxopts::value<std::string>()->default_value(""));

    // Group: General
    options.add_options("General")("help", "Print this help message")("no-output",
            "Process video without outputting ASCII animation to stdout",
            cxxopts::value<bool>()->default_value("false"));

#if defined(DEBUG_MODE) || defined(RELEASE_WITH_DEBUG_INF)
    std::clog << "Hallo Ich muss argc ihrgendwie nutzen deshalb hier der Wert " + std::to_string(argc) << '\n';

    std::vector<std::string> mock_argv = {argv[0],// argv[0] muss existieren!
            "--input", "/home/lupo/CLionProjects/img_to_ascii/funny.gif",
        "--width", "500", "--no-audio", "--no-output"};

    std::vector<const char*> argv_ptrs;

    argv_ptrs.reserve(mock_argv.size());

    // Wir konvertieren jeden std::string zu const char*
    for (const auto& arg: mock_argv) {
        argv_ptrs.emplace_back(arg.c_str());
        // c_str() gibt Pointer auf internen String zurück
    }

    int mock_argc = static_cast<int>(argv_ptrs.size());
    auto mock_argv_ptr = const_cast<char**>(argv_ptrs.data());
    // const_cast nötig weil cxxopts kein const akzeptiert

    const cxxopts::ParseResult parse_result = options.parse(mock_argc, mock_argv_ptr);
#else
    const cxxopts::ParseResult parse_result = options.parse(argc, argv);
#endif

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
    const int left_pad = parse_result["left-pad"].as<int>();

    // Saves the width and height in this pair where the width is the first and the height the second element
    const int image_dimensions_w = parse_result["w"].as<int>();
    const int image_dimensions_h = parse_result["h"].as<int>();

    const int frame_rate = parse_result["f"].as<int>();
    const bool no_color = parse_result["no-color"].as<bool>();
    const bool no_audio = parse_result["no-audio"].as<bool>();
    std::u32string charset32;
    utf8::utf8to32(charset.begin(), charset.end(), std::back_inserter(charset32));

    // Bestimmt den Output-Modus anhand der Flags: --output-file hat Vorrang vor --no-output
    auto output_mode = DataStructures::Output::STDOUT;
    std::string output_file;
    if (const auto& of = parse_result["output-file"].as<std::string>(); !of.empty()) {
        output_mode = DataStructures::Output::FILE;
        output_file = of;
    } else if (parse_result["no-output"].as<bool>()) {
        output_mode = DataStructures::Output::NO_OUTPUT;
    }

    if (input.empty()) {
        std::cerr << "Input video file is required" << std::endl;
        return 1;
    }
    if (!std::filesystem::exists(input) || !std::filesystem::is_regular_file(input)) {
        std::cerr << "Input is invalid" << std::endl;
        return 1;
    }
    if (frame_rate < 0) {
        std::cerr << "Frame rate is invalid" << std::endl;
        return 1;
    }
    if (image_dimensions_h <= 0 && image_dimensions_w <= 0) {
        std::cerr << "Height and width are invalid please provide at least one of them as positive integer"
                  << std::endl;
        return 1;
    }

    std::cout << "Input video file: " << input << '\n'
              << "Frame rate: " << (frame_rate > 0 ? std::to_string(frame_rate) : "use source") << '\n'
              << "Charset: " << charset << '\n'
              << "Width: " << image_dimensions_w << '\n'
              << "Height: " << image_dimensions_h << '\n'
              << "Left Pad: " << left_pad << '\n'
              << "No Color: " << (no_color ? "true" : "false") << '\n'
              << "Output: " << (output_mode == DataStructures::Output::FILE ? "file" : (output_mode == DataStructures::Output::STDOUT ? "stdout" : "none")) << '\n'
              << "Output File: " << output_file << '\n'
              << "No Audio: " << (no_audio ? "true" : "false") << '\n'
              << "Starting video to ascii conversion..." << std::endl;

    std::unique_ptr<AudioPlayer> audio = nullptr;

    if (no_audio) {
        DEBUG("Audio playback is disabled.");
    } else {
        try {
            DEBUG("Audio playback is enabled.");
            audio = std::make_unique<AudioPlayer>();
            audio->load(input);
            DEBUG("Audio file loaded successfully.");
        } catch (std::runtime_error& e) {
            std::cerr << e.what() << '\n'
                      << "Proceeding without Audio\n";
            audio->stop();
            audio->unload();
            audio.reset();
            audio = nullptr;
        }
    }

    DEBUG("Configure Renderer");
    OutputWriter output(output_mode, output_file);

    Renderer renderer(no_audio, output_mode == DataStructures::Output::NO_OUTPUT, audio.get(), frame_rate, &output);
    renderer.config.color = !no_color;
    renderer.set_left_pad(left_pad);
    if (!charset32.empty()) { renderer.set_charset(charset32); }
    DEBUG("Configured Renderer");

    DEBUG("Starting frame generation and output...");
    {
        if (output_mode == DataStructures::Output::STDOUT) {
            CursorGuard cursor_guard;
            RawOutputGuard raw_output_guard;
        }


        renderer.start_rendering();
        FrameHandler handler{.renderer = renderer};
        Renderer::decode_frames(input, frame_rate, image_dimensions_w, image_dimensions_h, handler);

        renderer.stop();
        output.stop();
    }
    DEBUG("Frame generation and output completed.");

    if (audio) {
        DEBUG("Cleaning up audio resources...");

        audio->stop();
        audio->unload();
        audio.reset();
        audio = nullptr;

        DEBUG("Audio unloaded");
    } else {
        DEBUG("Audio playback was disabled, no cleanup needed.");
    }

    std::cout << "Bye :3" << std::endl;
    return 0;
}
