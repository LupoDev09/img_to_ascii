// System header
#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <vector>
#include <chrono>

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

std::vector<unsigned char> load_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    return std::vector<unsigned char>(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}

int render_frame_ascii(const unsigned char *img, const int width, const int height, int target_width, int target_height, const bool use_color, const bool no_output) {
    constexpr float y_aspect = 2.0f;

    bool width_set  = target_width  > 0;
    bool height_set = target_height > 0;

    // Default
    if (!width_set && !height_set) {
        target_width = 55;
        width_set = true;
    }

    // calculate scale x and scale y
    verbose("start calculating scales in render_frame_ascii");
    float scale_x;
    float scale_y;

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

    verbose("start rendering frame");
    for (int y = 0; y < target_height; ++y) {
        std::string line;
        line.reserve(target_width * (use_color ? 20 : 1));

        for (int x = 0; x < target_width; ++x) {
            const int src_x = std::min(static_cast<int>(x * scale_x), width  - 1);
            const int src_y = std::min(static_cast<int>(y * scale_y), height - 1);

            const int idx = (src_y * width + src_x) * 3;

            const unsigned char r = img[idx];
            const unsigned char g = img[idx + 1];
            const unsigned char b = img[idx + 2];

            const char ascii = brightness_to_ascii(r, g, b);
            line.append(convert_to_ascii(ascii, r, g, b, use_color));
        }

        if (!no_output)
            std::cout << line << '\n';
    }
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
    ("no-output", "render but do not print anything to the console");

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

        // color
        std::string color_str = choices.count("color") ? "yes" : "no";

        // output
        std::string output_str = choices.count("no-output") ? "no" : "yes";

        // ASCII
        std::string ascii_str = choices["ascii"].as<std::string>();

        // zusammenbauen
        oss << "choices:\n"
            << "\t  img   = " << img_str << '\n'
            << "\t  width = " << width_str << '\n'
            << "\t  height= " << height_str << '\n'
            << "\t  color = " << color_str << '\n'
            << "\t  output = " << output_str << '\n'
            << "\t  ascii = " << ascii_str << '\n';

        verbose(oss.str());
    }

    // because the default image is set in the option we cann ignore the case that img is not provided
    const std::string img_path = choices["img"].as<std::string>();
    ASCII = choices["ascii"].as<std::string>();
    const bool use_color = choices.count("color") > 0;
    int target_width  = choices["width"].as<int>();
    int target_height = choices["height"].as<int>();

    auto lower = img_path;
    std::ranges::transform(lower, lower.begin(), ::tolower);
    bool is_gif = lower.ends_with(".gif");

    if (is_gif) {
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

        for (int f = 0; f < frames; ++f) {
            if (!choices.count("no-output")) {
                std::cout << "\033[H";   // Cursor Home
                std::cout << "\033[J";   // Clear screen
            }
            unsigned char* frame =
                gif + f * width * height * 3;

            int rendered_height = render_frame_ascii(
                frame,
                width,
                height,
                target_width,
                target_height,
                use_color,
                choices.count("no-output")
            );

            // Frame delay (GIF uses 1/100 sec)
            int delay_ms = delays ? delays[f] * 10 : 100;
            std::this_thread::sleep_for(
                std::chrono::milliseconds(delay_ms)
            );

            // Cursor hoch für Animation
            if (!choices.count("no-output")) {
                std::cout << "\033[" << rendered_height << "A";
                std::cout << "\r";
            }
        }
        std::cout << "\033[?25h";
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
