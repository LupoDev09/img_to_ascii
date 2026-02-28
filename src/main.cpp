// System header
#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <vector>
#include <chrono>
#include <algorithm>
#include <atomic>
#include <mutex>

// Provided header
#include <cxxopts.hpp>
#include <verbose.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// ASCII-character from dark → bright
std::string ASCII;

// for the progressbar
std::atomic<int> frames_done{0};
std::mutex cout_mutex;

/**
 * @brief constructs a GIF object to automatically destroy if the program ends
 * @author Lupo
 */
struct GifCleanup {
    unsigned char* gif = nullptr;
    int* delays = nullptr;

    // Deconstructor to clean up afterward
    ~GifCleanup() {
        std::cout << "\033[0m\033[?25h" << std::flush;
        if (gif) STBI_FREE(gif);
        if (delays) STBI_FREE(delays);
    }
};

/**
 * @brief constructs a Cursor object to automatically hide and show the cursor if the program ends
 * @author Lupo
 */
struct CursorGuard {
    CursorGuard()  { std::cout << "\033[?25l"; }
    ~CursorGuard() { std::cout << "\033[?25h\033[0m"; }
};

/**
 * @brief converts the brightness from the img to the corresponding ascii value from the ASCII var
 * @param r the value for red
 * @param g the value for green
 * @param b the value for blue
 * @return returns the corresponding char from the global ASCII var
 * @author Lupo
 */
inline char brightness_to_ascii(const unsigned char r, const unsigned char g, const unsigned char b) {
    // Wahrnehmung-korrekte Helligkeit
    const float brightness = 0.2126f * static_cast<float>(r) + 0.7152f * static_cast<float>(g) + 0.0722f * static_cast<float>(b);

    if (ASCII.empty()) throw std::runtime_error("ASCII alphabet can not be empty");

    const float t = brightness / 255.0f;
    const std::size_t max_idx = ASCII.size() - 1;
    const std::size_t index = static_cast<std::size_t>(std::clamp(t * static_cast<float>(max_idx), 0.0f, static_cast<float>(max_idx)));

    return ASCII[index];
}

/**
 * @brief output helper-funktion to convert to ascii
 * @param c the char to use
 * @param r the color value for red
 * @param g the color value for green
 * @param b the color value for blue
 * @param color if the output should be colored
 * @author Lupo
 */
inline std::string convert_to_ascii(const char c, const unsigned char r,
    const unsigned char g, const unsigned char b, const bool color) {
    std::string output;
    if (color) {
        // 24-bit Truecolor: \033[38;2;<r>;<g>;<b>m
        output.append("\033[38;2;")
      .append(std::to_string(r)).append(";")
      .append(std::to_string(g)).append(";")
      .append(std::to_string(b)).append("m")
      .append(1, c)
      .append("\033[0m");
    } else {
        output = std::string(1, c);
    }
    return output;
}

/**
 *
 * @param path the path to load the file from
 * @return a vector with the frames
 * @author Lupo
 */
inline std::vector<unsigned char> load_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
    }
    return std::vector<unsigned char>{
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    };
}

/**
 * @brief utility for render_frame_ascii_to_string
 * @param out the refrence to the output string
 * @param img the image to rander from
 * @param y the y posison to render from
 * @param width the width to render
 * @param height the height to render
 * @param target_width the targeted width to render
 * @param scale_x the x scale to render
 * @param scale_y the y scale to render
 * @param use_color wheather to use color
 */
inline void render_ascii_line(std::string& out, const unsigned char* img, const int y, const int width, const int height,
    const int target_width, const float scale_x, const float scale_y, const bool use_color) {
    out.clear();
    out.reserve(target_width * (use_color ? 10 : 1));

    for (int x = 0; x < target_width; ++x) {
        const int src_x = std::min(
            static_cast<int>(static_cast<float>(x) * scale_x),
            width - 1
        );
        const int src_y = std::min(
            static_cast<int>(static_cast<float>(y) * scale_y),
            height - 1
        );

        const int idx = (src_y * width + src_x) * 3;

        const unsigned char r = img[idx];
        const unsigned char g = img[idx + 1];
        const unsigned char b = img[idx + 2];

        const char ascii = brightness_to_ascii(r, g, b);
        out.append(convert_to_ascii(ascii, r, g, b, use_color));
    }
}


/**
 * @param img the image/frame to render
 * @param width the actual width from the image/frame
 * @param height the actual height from the image/frame
 * @param target_width the targeted width in the consol
 * @param target_height the targeted height in the consol
 * @param use_color weather to use color in the production to rander
 * @return a whole frame
 * @author Lupo
 */
std::string render_frame_ascii_to_string(const unsigned char *img, const int width, const int height,
    int target_width, int target_height, const bool use_color) {
    constexpr float y_aspect = 2.0f;
    bool width_set  = target_width  > 0;
    const bool height_set = target_height > 0;

    if (!width_set && !height_set) {
        target_width = 55;
        width_set = true;
    }

    float scale_x, scale_y;
    if (width_set && height_set) {
        scale_x = static_cast<float>(width)  / static_cast<float>(target_width);
        scale_y = static_cast<float>(height) / static_cast<float>(target_height);
    } else if (width_set) {
        scale_x = static_cast<float>(width) / static_cast<float>(target_width);
        scale_y = scale_x * y_aspect;
        target_height = static_cast<int>(static_cast<float>(height) / scale_y);
    } else {
        scale_y = static_cast<float>(height) / static_cast<float>(target_height);
        scale_x = scale_y / y_aspect;
        target_width = static_cast<int>(static_cast<float>(width) / scale_x);
    }

    std::vector<std::string> lines(target_height);
    const unsigned int max_threads = std::max(1u, std::thread::hardware_concurrency());
    const unsigned int threads_used = std::min<unsigned int>(target_height, max_threads);
    std::vector<std::thread> threads;
    const int chunk_size = std::max(1, target_height / static_cast<int>(threads_used));

    const bool use_threads = target_height >= 200;

    threads.reserve(threads_used);

    if (use_threads) {
        auto render_chunk = [&](const int start_y, const int end_y) {
            for (int y = start_y; y < end_y; ++y) {
                try {
                    render_ascii_line(lines[y], img, y, width, height, target_width, scale_x, scale_y, use_color);
                } catch (std::exception& e) {
                    std::cerr << e.what() << std::endl;
                }
            }
        };
        for (int start_y = 0; start_y < target_height; start_y += chunk_size) {
            int end_y = std::min(start_y + chunk_size, target_height);
            threads.emplace_back(render_chunk, start_y, end_y);
        }
        for (auto &t : threads) t.join();
    } else {
        for (int y = 0; y < target_height; ++y) {
            render_ascii_line(lines[y], img, y, width, height, target_width, scale_x, scale_y, use_color);
        }
    }
    std::string frame;
    frame.reserve(target_height * (target_width + 1));
    for (auto& line : lines) {
        frame.append(line);
        frame.push_back('\n');
    }
    return frame;
}

/**
 *
 * @param frame the frame to put out
 * @param no_output whether to provide output in the console
 * @param write_output_to_file whether to write to a file
 * @param file the file stream to write to
 * @author Lupo
 */
void output_frame(const std::string& frame, const bool no_output, const bool write_output_to_file, std::ofstream* file = nullptr) {
    if (write_output_to_file) {
        if (!file || !file->is_open()) {
            throw std::runtime_error("Output file not open");
        }
        img_to_ascii::verbose("Trying to write to file");
        *file << frame << '\n';
        img_to_ascii::verbose("Wrote to file");
        return;
    }

    if (!no_output) {
        std::cout << "\033[H\033[J";
        std::cout << frame << std::flush;
        img_to_ascii::verbose("Wrote to console");
        return;
    }
    img_to_ascii::verbose("No output selected");
}

/**
 *
 * @param path the image to load
 * @param target_width the targeted width for the output
 * @param target_height the targeted height for the output
 * @param use_color whether to use ansi-escapes for color
 * @return a string with the rendered image
 */
std::string render_image(const std::string& path, const int target_width, const int target_height, const bool use_color) {
    int width, height;
    img_to_ascii::verbose("trying to load image");
    unsigned char* img = stbi_load(path.c_str(), &width, &height, nullptr, 3);

    if (!img) {
        throw std::runtime_error(
            "Failed to load image: " + std::string(stbi_failure_reason())
        );
    }
    img_to_ascii::verbose("Loaded image");

    img_to_ascii::verbose("trying to rander frame");
    std::string result = render_frame_ascii_to_string(
        img,
        width,
        height,
        target_width,
        target_height,
        use_color
    );

    stbi_image_free(img);
    return result;
}


int main(const int argc, char** argv) {
    cxxopts::Options options("img_to_ascii", "Convert images and GIFs to ASCII art");

    // Help
    options.add_options("General")
        ("help", "produce help message")
        ("img", "The image to load", cxxopts::value<std::string>()->default_value("Silly_Cat_Character.jpg"));

    // Optional
    options.add_options("Output")
        ("w,width", "Target output width (0 = auto)", cxxopts::value<int>()->default_value("0"))
        ("h,height", "Target output height (0 = auto)", cxxopts::value<int>()->default_value("0"))
        ("c,color", "Enable ANSI truecolor output")
        ("ascii", "Characters used for brightness mapping (dark → bright)", cxxopts::value<std::string>()->default_value("@%#*+=-:. "))
        ("write-output-to-file", "write the generated image/frames to a file rather than to the console")
        ("file", "specify the file to write to", cxxopts::value<std::string>()->default_value("image.txt"));

    // GIF-specific
    options.add_options("GIF")
        ("f,fps", "Force frames per second (0 = Use GIF Timing)", cxxopts::value<int>()->default_value("0"))
        ("l,loop", "how often the gif should replay", cxxopts::value<int>()->default_value("0"));

    options.add_options("Debugging")
        ("v,verbose", "activate verbose mode")
        ("no-output", "render but do not print anything to the console");

    const cxxopts::ParseResult choices = options.parse(argc, argv);
    // handle help flag
    if (choices.count("help")) {
        // Basic groups
        std::cout << options.help({"General", "Output"}) << "\n";
        std::cout << "These options don’t do anything if used with normal images";

        // GIF & Debug, aber ohne Header/Usage
        std::stringstream ss(options.help({"GIF", "Debugging"}, false));
        std::string line;
        bool first_line = true;
        while (std::getline(ss, line)) {
            if (first_line) { first_line = false; continue; } // erste Zeile (Programmnamen) skippen
            std::cout << line << "\n";
        }

        // Authors note
        std::cout << "\nNote:" 
                  << "\nI can't recommend opening the file written from this tool when the color option was provided, "
                  << "because the file will be mostly ansi-escapes."
                  << "\nUse something like `cat` to write the file to the console (that also works with colors) :3"
                  << "\nThe same goes for GIFs regardless of color this will only produce the frames :3";
            return 0;
    }

    if (choices.count("verbose")) img_to_ascii::VERBOSE_MODE = true;

    {
        std::ostringstream oss;

        // img
        const std::string img_str = choices["img"].as<std::string>();


        // width
        int width_val = choices["width"].as<int>();
        const std::string width_str = (width_val == 0) ? "not provided using default" : std::to_string(width_val);

        // height
        int height_val = choices["height"].as<int>();
        const std::string height_str = (height_val == 0) ? "not provided using default" : std::to_string(height_val);

        // color
        const std::string color_str = choices.count("color") ? "yes" : "no";

        // ASCII
        const std::string ascii_str = choices["ascii"].as<std::string>();

        // generate output
        const std::string generate_output_str = choices.count("no-output") ? "no" : "yes";

        // write output to file
        const std::string write_output_to_file_str = choices.count("write-output-to-file") ? "yes" : "no";

        // write output to specified file
        const std::string write_output_to_specific_file = choices["file"].as<std::string>();

        // FPS
        int fps_val = choices["fps"].as<int>();
        const std::string fps_overwrite_str = (fps_val == 0) ? "not provided using default" : std::to_string(fps_val);

        // loop
        int loop_val = choices["loop"].as<int>();
        const std::string loop_str = (loop_val > 0) ? std::to_string(loop_val): "Using default value";

        // zusammenbauen
        oss << "choices:\n"
            << "\t  img                  = " << img_str << '\n'
            << "\t  width                = " << width_str << '\n'
            << "\t  height               = " << height_str << '\n'
            << "\t  fps                  = " << fps_overwrite_str << '\n'
            << "\t  color                = " << color_str << '\n'
            << "\t  generate output      = " << generate_output_str << '\n'
            << "\t  write output to file = " << write_output_to_file_str << '\n'
            << "\t  specified file       = " << write_output_to_specific_file << '\n'
            << "\t  ASCII                = " << ascii_str << '\n'
            << "\t  loop                 = " << loop_str << '\n';

        img_to_ascii::verbose(oss.str());
    }

    // setting values from the CLI Part
    const std::string img_path = choices["img"].as<std::string>();
    ASCII = choices["ascii"].as<std::string>();
    const bool use_color = choices.count("color") > 0;
    const bool no_output = choices.count("no-output") > 0;
    const bool write_output_to_file = choices.count("write-output-to-file") > 0;
    const std::string file_path = choices["file"].as<std::string>();
    int target_width  = choices["width"].as<int>();
    int target_height = choices["height"].as<int>();
    int fps_override = choices["fps"].as<int>();
    int loops = choices["loop"].as<int>();

    if (loops < 0) {
        std::cerr << "loop has to be at least 0 using default 0" << std::endl;
        loops = 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }

    if (fps_override <= 0) {
        img_to_ascii::verbose("fps override can not be 0 or lower. Setting it to default");
    }

    if (target_height == 0 && target_width == 0) {
        target_width = 80;
        target_height = 45;
    }

    auto lower = img_path;
    for (char& c : lower)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (lower.ends_with(".gif")) {
        img_to_ascii::verbose("Detected GIF");
        auto data = load_file(img_path);

        int* delays = nullptr;
        int frames = 0, width = 0, height = 0;

        unsigned char* gif = stbi_load_gif_from_memory(
            data.data(),
            static_cast<int>(data.size()),
            &delays,
            &width,
            &height,
            &frames,
            nullptr,
            3
        );

        GifCleanup gif_cleanup;
        gif_cleanup.gif = gif;
        gif_cleanup.delays = delays;

        if (!gif || frames <= 0) {
            std::cerr << "Failed to load gif '" << img_path
                  << "': " << stbi_failure_reason() << std::endl;
            return 1;
        }

        img_to_ascii::verbose("GIF loaded, frames: " + std::to_string(frames));

        img_to_ascii::verbose("Generating frames...");
        // Pre-process: alle Frames in Strings rendern
        std::vector<std::string> processed_frames(frames);
        const unsigned int max_threads = std::max(1u, std::thread::hardware_concurrency());
        std::vector<std::jthread> threads;

        CursorGuard cursor;
        frames_done.store(0);
        // lambda funktion für die threads
        auto render_chunk = [&](const std::stop_token& st, const int start_f, const int end_f) {
            try {
                for (int f = start_f; f < end_f && !st.stop_requested(); ++f) {
                    const unsigned char* frame = gif + f * width * height * 3;

                    // Nutze Chunked-Multithreading-Version für die Zeilen
                    processed_frames[f] = render_frame_ascii_to_string(
                        frame, width, height,
                        target_width, target_height,
                        use_color
                    );

                    // Fortschritt erhöhen
                    ++frames_done;

                    // Fortschrittsanzeige anzeigen
                    {
                        const int progress = static_cast<int>((frames_done.load() * 100) / frames);
                        std::lock_guard<std::mutex> lock(cout_mutex);
                        std::cout << "\rGenerating frames: ["
                                  << std::string(progress / 2, '=')
                                  << std::string(50 - progress / 2, ' ')
                                  << "] " << progress << "% "
                                  << std::flush;
                        if (frames_done.load() == frames) {
                            std::cout << std::endl;
                        }
                    }
                }
            } catch (std::exception& e) {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cerr << "Thread error: " << e.what() << std::endl;
            }
        };
        std::cout << '\n' << std::endl;

        // Thread-Chunking über Frames
        int chunk_size = std::max(1, frames / static_cast<int>(max_threads));
        for (int start = 0; start < frames; start += chunk_size) {
            int end = std::min(start + chunk_size, frames);
            threads.emplace_back([start, end, &render_chunk](const std::stop_token& st){ render_chunk(st, start, end); });
        }
        // wir müssen nicht auf die threads warten und joinen, weil es sich um jthreads handelt

        // Jetzt Ausgabe
        img_to_ascii::verbose("hiding cursor");
        using clock = std::chrono::steady_clock;
        auto next_frame_time = clock::now();

        img_to_ascii::verbose("Trying to open file");
        std::fstream img_text_file;
        img_text_file.open("image.txt", std::ios::out);
        if (!img_text_file.is_open()) {
            img_to_ascii::verbose("Failed to open file");
            return 1;
        }
        img_to_ascii::verbose("printing frames");
        for (int loop_i = 0; loop_i <= loops; ++loop_i) {
            for (int f = 0; f < frames; ++f) {
                if (!write_output_to_file) {
                    if (!no_output) {
                        std::cout << "\033[H\033[J"; // Cursor Home + Clear Screen
                        std::cout << processed_frames[f]; // Frame ausgeben
                        std::cout.flush();
                    }

                    // FPS Steuerung
                    if (fps_override > 0) {
                        auto frame_duration = std::chrono::milliseconds(1000 / fps_override);
                        next_frame_time += frame_duration;
                        std::this_thread::sleep_until(next_frame_time);
                    } else {
                        int delay_ms = delays ? std::max(1, delays[f] * 10) : 100;
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                    }
                } else {
                    img_to_ascii::verbose("writing output to file");

                    if (img_text_file.is_open()) {
                        img_text_file << processed_frames[f] << std::endl;
                        img_to_ascii::verbose("wrote output to file");
                    } else {
                        std::cerr << "Error while creating/opening file" << std::endl;
                        return 1;
                    }
                }
            }
        }
        return 0;
    }

    img_to_ascii::verbose("Detected IMAGE");

    CursorGuard cursor;
    std::ofstream file;
    if (write_output_to_file) {
        img_to_ascii::verbose("trying to open file");
        file.open(file_path, std::ios::out);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open output file");
        }
        img_to_ascii::verbose("Opened file");
    }

    img_to_ascii::verbose("Rendering Frame");
    std::string frame = render_image(
        img_path,
        target_width,
        target_height,
        use_color
    );

    img_to_ascii::verbose("outputting frame");
    output_frame(
        frame,
        no_output,
        write_output_to_file,
        write_output_to_file ? &file : nullptr
    );

    img_to_ascii::verbose("program ends");
    return 0;
}
