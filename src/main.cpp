// System header
#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <vector>
#include <chrono>
#include <algorithm>

// Provided header
#include <cxxopts.hpp>
#include "verbose.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// ASCII-character from dark → bright
std::string ASCII;

/**
 * @brief converts the brightness from the img to the corresponding ascii value from the ASCII var
 * @param r the value for red
 * @param g the value for green
 * @param b the value for blue
 * @return returns the corresponding char from the global ASCII var
 */
inline char brightness_to_ascii(const unsigned char r, const unsigned char g, const unsigned char b) {
    // Wahrnehmungs-korrekte Helligkeit
    const float brightness = 0.2126f * static_cast<float>(r) + 0.7152f * static_cast<float>(g) + 0.0722f * static_cast<float>(b);
    const unsigned long  index = (brightness / 255.0f) * (ASCII.size() - 1); // Do not fix this casting issue it will brake everything :3
    return ASCII[index];
}

/**
 * @brief output helper-funktion to convert to ascii
 * @param c the char to use
 * @param r the color value for red
 * @param g the color value for green
 * @param b the color value for blue
 * @param color if the output should be colored
 */
inline std::string convert_to_ascii(const char c, const unsigned char r, const unsigned char g, const unsigned char b, const bool color) {
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
 */
std::vector<unsigned char> load_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    return std::vector<unsigned char>(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}

/**
 * @param img the image/frame to render
 * @param width the actual width from the image/frame
 * @param height the actual height from the image/frame
 * @param target_width the targeted width in the consol
 * @param target_height the targeted height in the consol
 * @param use_color weather to use color in the production to rander
 * @param no_output weather to put the rendered stuff on the consol
 * @return
 */
int render_frame_ascii(const unsigned char *img, const int width, const int height, int target_width, int target_height, const bool use_color, const bool no_output) {
    constexpr float y_aspect = 2.0f;

    bool width_set  = target_width  > 0;
    const bool height_set = target_height > 0;

    // Default
    if (!width_set && !height_set) {
        target_width = 55;
        width_set = true;
    }

    // calculate scale x and scale y
    float scale_x, scale_y;

    if (width_set && height_set) {
        scale_x = static_cast<float>(width)  / target_width;
        scale_y = static_cast<float>(height) / target_height;
    } else if (width_set) {
        scale_x = static_cast<float>(width) / target_width;
        scale_y = scale_x * y_aspect;
        target_height = static_cast<int>(height / scale_y);
    } else {
        scale_y = static_cast<float>(height) / target_height;
        scale_x = scale_y / y_aspect;
        target_width = static_cast<int>(width / scale_x);
    }

    // Speicher für die Zeilen
    std::vector<std::string> lines(target_height);

    // Anzahl Threads auf Hardware-Kerne beschränken
    const unsigned int max_threads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;

    for (int y = 0; y < target_height; ++y) {
        // Falls wir zu viele Threads haben, warten wir, bis welche fertig sind
        while (threads.size() >= max_threads) {
            threads.front().join();
            threads.erase(threads.begin());
        }

        threads.emplace_back([&, y]() {
            std::string line;
            line.reserve(target_width * (use_color ? 10 : 1)); // reservieren für Farben
            for (int x = 0; x < target_width; ++x) {
                const int src_x = std::min(static_cast<int>(x * scale_x), width - 1);
                const int src_y = std::min(static_cast<int>(y * scale_y), height - 1);
                const int idx = (src_y * width + src_x) * 3;

                const unsigned char r = img[idx];
                const unsigned char g = img[idx + 1];
                const unsigned char b = img[idx + 2];

                const char ascii = brightness_to_ascii(r, g, b);
                line.append(convert_to_ascii(ascii, r, g, b, use_color));
            }
            lines[y] = line;
        });
    }

    // Alle Threads fertig machen lassen
    for (auto &t : threads) t.join();

    // Zeilen zusammenfügen
    std::string frame;
    for (auto &line : lines) frame += line + "\n";

    if (!no_output) std::cout << frame;

    return target_height;
}


int main(const int argc, char** argv) {
    cxxopts::Options options("img_to_ascii", "My try to rewrite my img to ascii tool");
    options.add_options()
    ("help", "produce help message")
    ("img", "The image to load", cxxopts::value<std::string>()->default_value("Silly_Cat_Character.jpg"))
    ("w,width", "Target output width", cxxopts::value<int>()->default_value("0"))
    ("h,height", "Target output height", cxxopts::value<int>()->default_value("0"))
    ("c,color", "Enable ANSI truecolor output")
    ("ascii", "change the ASCII alphabet to use from dark -> bright ", cxxopts::value<std::string>()->default_value("@%#*+=-:. "))
    ("v, verbose", "activate verbose mode")
    ("no-output", "render but do not print anything to the console")
    ("fps", "Force frames per second (overrides GIF timing)", cxxopts::value<int>()->default_value("0"))
    ("loop", "how often the gif should replay", cxxopts::value<int>()->default_value("0"));

    // setting values from the CLI Part
    const auto choices = options.parse(argc, argv);
    if (choices.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    if (choices.count("verbose")) VERBOSE_MODE = true;

    {
        std::ostringstream oss;

        // img
        std::string img_str = choices["img"].as<std::string>();

        // width
        int width_val = choices["width"].as<int>();
        std::string width_str = (width_val == 0) ? "not provided using default" : std::to_string(width_val);

        // height
        int height_val = choices["height"].as<int>();
        std::string height_str = (height_val == 0) ? "not provided using default" : std::to_string(height_val);

        // FPS
        int fps_val = choices["fps"].as<int>();
        std::string fps_overide_str = (fps_val == 0) ? "not provided using default" : std::to_string(fps_val);

        // color
        std::string color_str = choices.count("color") ? "yes" : "no";

        // output
        std::string output_str = choices.count("no-output") ? "no" : "yes";

        // ASCII
        std::string ascii_str = choices["ascii"].as<std::string>();

        // loop
        int loop_val = choices["loop"].as<int>();
        std::string loop_str = (loop_val > 0) ? std::to_string(loop_val): "Using default value";

        // zusammenbauen
        oss << "choices:\n"
            << "\t  img   = " << img_str << '\n'
            << "\t  width = " << width_str << '\n'
            << "\t  height= " << height_str << '\n'
            << "\t  fps   = " << fps_overide_str << '\n'
            << "\t  color = " << color_str << '\n'
            << "\t  output = " << output_str << '\n'
            << "\t  ascii = " << ascii_str << '\n'
            << "\t  loop  = " << loop_str << '\n';

        verbose(oss.str());
    }

    // because the default image is set in the option we cann ignore the case that img is not provided
    const std::string img_path = choices["img"].as<std::string>();
    ASCII = choices["ascii"].as<std::string>();
    const bool use_color = choices.count("color") > 0;
    int target_width  = choices["width"].as<int>();
    int target_height = choices["height"].as<int>();
    int fps_override = choices["fps"].as<int>();
    int loops = choices["loop"].as<int>();
    if (loops < 0) {
        std::cerr << "loop has to be at least 0 using default 0" << std::endl;
        loops = 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }

    auto lower = img_path;
    std::ranges::transform(lower, lower.begin(), ::tolower);

    if (lower.ends_with(".gif")) {
        verbose("Detected GIF");

        // Load file
        auto data = load_file(img_path);

        // set defaults
        int* delays = nullptr;
        int frames = 0;
        int width, height;

        // load it from memory to be able to use it
        unsigned char* gif = stbi_load_gif_from_memory(
            data.data(),
            data.size(),
            &delays,
            &width,
            &height,
            &frames,
            nullptr,
            3
        );

        if (!gif) {
            std::cerr << "Failed to load GIF\n";
            return 1;
        }

        verbose("GIF loaded, frames: " + std::to_string(frames));
        verbose("hiding cursor");
        std::cout << "\033[?25l";
        using clock = std::chrono::steady_clock;
        auto next_frame_time = clock::now();
        int rendered_height;

        if (loops > 0) {
            verbose(std::string("Will render ") + std::to_string(loops + 1) + " times");
        }
        // loop mindestens 1-mal aber bis zu loops
        for (int _ = 0; _ < loops+1; _++) {
            for (int f = 0; f < frames; ++f) {
                if (!choices.count("no-output")) {
                    std::cout << "\033[H";   // Cursor Home
                    std::cout << "\033[J";   // Clear screen
                }
                unsigned char* frame = gif + f * width * height * 3;

                rendered_height = render_frame_ascii(
                    frame,
                    width,
                    height,
                    target_width,
                    target_height,
                    use_color,
                    choices.count("no-output")
                );

                // Cursor hoch für Animation
                if (f < frames - 1 && !choices.count("no-output")) {
                    std::cout << "\033[" << rendered_height << "A";
                    std::cout << "\r";
                }
                // FPS
                if (fps_override > 0) {
                    next_frame_time += std::chrono::milliseconds(1000 / fps_override);
                    std::this_thread::sleep_until(next_frame_time);
                } else {
                    int delay_ms = delays ? delays[f] * 10 : 100;
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                }
                if (!choices.count("no-output")) {
                    // Cursor unter das letzte Frame setzen
                    std::cout << "\033[" << rendered_height << "B";

                    // Neue Zeile, damit Shell nicht im Bild landet
                    std::cout << '\n';
                }
            }
        }
        // Animation finished
        if (!choices.count("no-output")) {
            std::cout << "\033[0m";                 // reset colors
            std::cout << "\033[" << rendered_height << "B";
            std::cout << '\n';
        }

        std::cout << "\033[?25h";
        std::cout.flush();

        verbose("Showing cursor again");

        STBI_FREE(gif);
        STBI_FREE(delays);
        verbose("program ends");
        return 0;
    }

    verbose("Trying to load image: " + img_path);
    int width, height, channels;
    unsigned char* img = stbi_load(img_path.c_str(), &width, &height, &channels, 3);

    if (!img) {
        std::cerr << "Error while loading the image from " << img_path << std::endl;
        return 1;
    }
    verbose("loaded image: " + img_path);

    render_frame_ascii(
        img,
        width,
        height,
        target_width,
        target_height,
        use_color,
        choices.count("no-output")
    );

    stbi_image_free(img);
    verbose("program ends");
    return 0;
}
