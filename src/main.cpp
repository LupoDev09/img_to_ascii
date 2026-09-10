#define MINIAUDIO_IMPLEMENTATION
// ReSharper disable once CppUnusedIncludeDirective
#include <miniaudio.h>

#include <AudioPlayer.hpp>
#include <GenerateFrames.hpp>
#include <OutputWriter.hpp>
#include <Renderer.hpp>
#include <SyncClock.hpp>
#include <Verbose.hpp>
#include <chrono>
#include <cxxopts.hpp>
#include <dataStructures.hpp>
#include <filesystem>
#include <iostream>
#include <utf8/checked.h>


namespace {
    // Makes the cursor invisible on construction and visible through a function call or at deconstruction
    struct CursorGuard {
        CursorGuard() {
            std::cout << "\033[?25l";// Hide cursor
        }

        static void makeVisible() {
            std::cout << "\033[?25h";// Show cursor
        }

        ~CursorGuard() {
            makeVisible();
        }
    };

    struct FrameHandler {
        Renderer& renderer;
        AudioPlayer* audio; // Not owning
        OutputWriter& output;
        SyncClock& clock;
        int frame_rate;
        unsigned int frame_index = 0;
        double target_ms = 0.0;
        bool no_output;
        bool no_audio;
        bool first_frame = true;

        void operator()(DataStructures::Frame&& frame) {
            // TODO: Rework the Architecture so the rendering is happening in it's own thread so we don't sleep during decoding
            if (first_frame) {
                clock.start();

                if (!no_output && !no_audio && audio != nullptr) {
                    audio->play();
                }

                const double source_fps = frame.source_fps;

                target_ms = frame_rate > 0
                    ? 1000.0 / frame_rate
                    : 1000.0 / source_fps;

                if (!no_output) {
                    output.start();
                    output.push("\033[2J\033[H");
                }

                first_frame = false;
            }

            const std::string rendered = renderer.render_frame(frame);

            if (!no_output) {
                output.push("\033[H");
                output.push(rendered);
            }

            const double expected = frame_index * target_ms;
            clock.wait_until(expected);
            frame_index++;
        }
    };
}

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
    ("c,charset", "Character palette for ASCII mapping (darker to lighter)", cxxopts::value<std::string>()->default_value(" ░▒▓█"))
    ("left-pad", "Left padding for each line of ASCII art", cxxopts::value<int>()->default_value("0"));

    // Group: General
    options.add_options("General")
    ("help", "Print this help message")
    ("no-output", "Process video without outputting ASCII animation to stdout", cxxopts::value<bool>()->default_value("false"));

#ifdef DEBUG_MODE
    DEBUG("Hallo Ich muss argc ihrgendwie nutzen deshalb hier der Wert " + std::to_string(argc));

    std::vector<std::string> mock_argv = {
        argv[0], // argv[0] muss existieren!
        "--input", "/home/lupo/CLionProjects/img_to_ascii/funny.gif",
        "--width", "50",
        "--no-audio"
    };

    std::vector<const char*> argv_ptrs;

    argv_ptrs.reserve(mock_argv.size());

    // Wir konvertieren jeden std::string zu const char*
    for (const auto& arg : mock_argv) {
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
    const std::pair image_dimensions = { parse_result["w"].as<int>(), parse_result["h"].as<int>() };

    const int frame_rate = parse_result["f"].as<int>();
    const bool no_output = parse_result["no-output"].as<bool>();
    const bool no_color = parse_result["no-color"].as<bool>();
    const bool no_audio = parse_result["no-audio"].as<bool>();
    std::u32string charset32;
    utf8::utf8to32(charset.begin(), charset.end(), std::back_inserter(charset32));

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
    if (image_dimensions.second <= 0 && image_dimensions.first<= 0) {
        std::cerr << "Height and width are invalid please provide at least one of them as positive integer" << std::endl;
        return 1;
    }

    std::cout << "Input video file: " << input << '\n'
              << "Frame rate: " << (frame_rate > 0 ? std::to_string(frame_rate) : "use source") << '\n'
              << "Charset: " << charset << '\n'
              << "Width: " << image_dimensions.first<< '\n'
              << "Height: " << image_dimensions.second << '\n'
              << "Left Pad: " << left_pad << '\n'
              << "No Color: " << (no_color ? "true" : "false") << '\n'
              << "No Output: " << (no_output ? "true" : "false") << '\n'
              << "No Audio: " << (no_audio ? "true" : "false") << '\n'
              << "Starting video to ascii conversion..." << std::endl;

    std::unique_ptr<AudioPlayer> audio = nullptr;
    SyncClock clock;

    if (no_audio) {
        DEBUG("Audio playback is disabled.");
    } else {
        DEBUG("Audio playback is enabled.");
        audio = std::make_unique<AudioPlayer>();
        audio->load(input);
        DEBUG("Audio file loaded successfully.");
    }

    DEBUG("Configure Renderer");
    Renderer renderer;
    renderer.config.color = !no_color;
    renderer.set_left_pad(left_pad);
    if (!charset32.empty()) {
        renderer.set_charset(charset32);
    }
    DEBUG("Configured Renderer");

    DEBUG("Starting frame generation and output...");
    CursorGuard cursor_guard;
    OutputWriter output;

    FrameHandler handler{.renderer = renderer, .audio = audio.get(), .output = output, .clock = clock, .frame_rate = frame_rate, .no_output = no_output, .no_audio = no_audio};
    GenerateFrames::generate(input, frame_rate, image_dimensions.first, image_dimensions.second, handler);

    output.stop();
    CursorGuard::makeVisible();
    DEBUG("Frame generation and output completed.");

    DEBUG("Cleaning up audio resources...");
    if (audio) {
        audio->stop();
        audio->unload();
    }
    DEBUG("Audio unloaded");

    std::cout << "Bye :3" << std::endl;
    return 0;
}
