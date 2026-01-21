#include <iostream>
#include <string>
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


int main(const int argc, char** argv) {
    cxxopts::Options options("img_to_ascii", "My try to rewrite my img to ascii tool");
    options.add_options()
    ("help", "produce help message")
    ("img", "The image to load", cxxopts::value<std::string>()->default_value("Silly_Cat_Character.jpg"))
    ("w,width", "Target output width", cxxopts::value<int>()->default_value("0"))
    ("h,height", "Target output height", cxxopts::value<int>()->default_value("0"))
    ("c,color", "Enable ANSI truecolor output")
    ("ascii", "change the ASCII alphabet to use from dark -> bright ", cxxopts::value<std::string>()->default_value("@%#*+=-:. "))
    ("v, verbose", "activate verbose mode");

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

        // ASCII
        std::string ascii_str = choices["ascii"].as<std::string>();

        // zusammenbauen
        oss << "choices:\n"
            << "\t  img   = " << img_str << '\n'
            << "\t  width = " << width_str << '\n'
            << "\t  height= " << height_str << '\n'
            << "\t  color = " << color_str << '\n'
            << "\t  ascii = " << ascii_str << '\n';

        verbose(oss.str());
    }

    // because the default image is set in the option we cann ignore the case that img is not provided
    const std::string img_path = choices["img"].as<std::string>();
    ASCII = choices["ascii"].as<std::string>();
    const bool use_color = choices.count("color") > 0;
    int target_width  = choices["width"].as<int>();
    int target_height = choices["height"].as<int>();

    int width, height, channels;
    unsigned char* img = stbi_load(img_path.c_str(), &width, &height, &channels, 3);

    if (!img) {
        std::cerr << "Error while loading the image from " << img_path << std::endl;
        return 1;
    }
    verbose("Loaded image");

    constexpr float y_aspect = 2.0f;
    bool width_set  = target_width  > 0;
    const bool height_set = target_height > 0;

    // Default
    if (!width_set && !height_set) {
        target_width = 55;
        width_set = true;
    }

    float scale_x;
    float scale_y;

    // Calculate target size
    if (width_set && height_set) {
        // Stretch (explict)
        scale_x = static_cast<float>(width)  / static_cast<float>(target_width);
        scale_y = static_cast<float>(height) / static_cast<float>(target_height);
    }
    else if (width_set) {
        scale_x = static_cast<float>(width) / static_cast<float>(target_width);
        scale_y = scale_x * y_aspect;
        target_height = static_cast<int>(static_cast<float>(height) / scale_y);
    }
    else {
        scale_y = static_cast<float>(height) / static_cast<float>(target_height);
        scale_x = scale_y / y_aspect;
        target_width = static_cast<int>(static_cast<float>(width) / scale_x);
    }
    verbose("Calculated target size");

    // Rendering
    verbose("start rendering");
    for (int y = 0; y < target_height; ++y) {
        std::string line;
        line.reserve(target_width * (use_color ? 20 : 1)); // Reserve space for the string per line
        for (int x = 0; x < target_width; ++x) {

            int src_x = static_cast<int>(static_cast<float>(x) * scale_x);
            int src_y = static_cast<int>(static_cast<float>(y) * scale_y);

            src_x = std::min(src_x, width  - 1);
            src_y = std::min(src_y, height - 1);

            const int index = (src_y * width + src_x) * 3;

            const unsigned char r = img[index];
            const unsigned char g = img[index + 1];
            const unsigned char b = img[index + 2];

            const char ascii = brightness_to_ascii(r, g, b);

            line.append(convert_to_ascii(ascii, r, g, b, use_color));
        }
        std::cout << line << '\n';
    }
    verbose("Rendered");

    stbi_image_free(img);
    return 0;
}
