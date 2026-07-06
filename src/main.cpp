#include <GenerateFrames.h>
#include <Renderer.h>
#include <dataStructures.h>
#include <cxxopts.hpp>
#include <miniaudio.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

namespace fs = std::filesystem;

bool VERBOSE_MODE = false;
#define VERBOSE(msg) if (VERBOSE_MODE) std::clog << msg << std::endl

void get_audio_file(const std::string& input_file, const std::string& output_file_path) {
    const std::string command =
    "ffmpeg -y -i \"" + input_file +
    "\" -vn -acodec pcm_s16le -ar 44100 -ac 2 \"" +
    output_file_path + "\"";
    system(command.c_str());
}

struct AudioEngin {
    ma_engine engine;

    ~AudioEngin() {
        ma_engine_uninit(&engine);
    }
};

/*
 * TODO: Improve Audio support
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
    ("c,charset", "Charset to use for ascii mapping", cxxopts::value<std::string>()->default_value(" ░▒▓█"));

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
    std::u32string charset32 = utf8_to_utf32(charset);
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

    std::cout << "Input video file: " << input << '\n'
              << "Frame rate: " << (frame_rate > 0 ? std::to_string(frame_rate) : "use source") << '\n'
              << "Charset: " << charset << '\n'
              << "Width: " << width << '\n'
              << "Height: " << height << '\n'
              << "Verbose: " << (verbose ? "true" : "false") << '\n'
              << "No Color: " << (no_color ? "true" : "false") << '\n'
              << "No Output: " << (no_output ? "true" : "false") << '\n'
              << "Starting video to ascii conversion..." << std::endl;

    std::string Audio_output_file_path = "audio.wav";
    get_audio_file(input, Audio_output_file_path);
    AudioEngin* engine = nullptr;
    if (fs::exists(Audio_output_file_path)) {
        engine = new AudioEngin();
        if (ma_engine_init(nullptr, &engine->engine) != MA_SUCCESS) {
            std::cerr << "Audio init failed\n";
            return -1;
        }
    }

    Renderer renderer;
    renderer.config.color = !no_color;
    if (!charset.empty()) {
        renderer.config.charset = charset32;
    }

    VERBOSE("Starting frame generation and output...");
    bool first_frame = true;
    double target_ms = 0.0; // Default value, will be overwritten below
    double source_fps = 0.0; // holds the fps from the first frame
    auto last_tick = std::chrono::steady_clock::now();
    GenerateFrames::generate(input_path, frame_rate, width, height,
        [&](DataStructures::Frame&& frame) {
            if (!first_frame && !no_output) {
                std::cout << "\033[2J\033[H"; // alten Frame löschen
            }

            if (first_frame) {
                source_fps = frame.source_fps; // Setz die Zeit die durchgängig genutzt wird zum Warten
                last_tick = std::chrono::steady_clock::now(); // setzt last_tick auf den Start des ganzen

                // Calculate the target time to wait
                target_ms = frame_rate > 0
                ? 1000.0 / frame_rate
                : 1000.0 / source_fps;

                first_frame = false;

                // Starte das playback, wenn die engine existiert
                if (engine != nullptr) {
                    ma_engine_play_sound(&engine->engine, Audio_output_file_path.c_str(), nullptr);
                }
            }

            const std::string rendered = renderer.render_frame(frame); // Rendert den frame in einen vector

            if (!no_output) {
                std::cout << rendered << std::flush; // output the frame
            }

            const auto now = std::chrono::steady_clock::now();
            const double elapsed_ms = std::chrono::duration<double, std::milli>(now - last_tick).count();

            if (elapsed_ms < target_ms) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(static_cast<int>(target_ms - elapsed_ms))
                );
            }

            last_tick = std::chrono::steady_clock::now();
    });

    if (engine != nullptr) {
        fs::remove(Audio_output_file_path);
    }
    delete engine;
    VERBOSE("Frame generation and output completed.");
    VERBOSE("Bye :3");
    return 0;
}
