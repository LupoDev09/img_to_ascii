#include <iostream>
#include <string>
#include <cxxopts.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// ASCII-Zeichen von dunkel → hell
const std::string ASCII = "@%#*+=-:. ";

inline char brightness_to_ascii(const unsigned char r, const unsigned char g, const unsigned char b) {
    // Wahrnehmungs-korrekte Helligkeit
    const float brightness = 0.2126f * static_cast<float>(r) + 0.7152f * static_cast<float>(g) + 0.0722f * static_cast<float>(b);
    const unsigned long  index = (brightness / 255.0f) * (ASCII.size() - 1);
    return ASCII[index];
}

int main(const int argc, char** argv) {
    cxxopts::Options options("img_to_ascii", "My try to rewrite my img to ascii tool");
    options.add_options()
    ("help", "produce help message")
    ("img", "The image to load", cxxopts::value<std::string>()->default_value("Silly_Cat_Character.jpg"))
    ("w,width", "Target output width", cxxopts::value<int>()->default_value("0"))
    ("h,height", "Target output height", cxxopts::value<int>()->default_value("0"));

    const auto choices = options.parse(argc, argv);
    std::string img_path;
    if (choices.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    if (choices.count("img")) {
        img_path = choices["img"].as<std::string>();
    } else {
        img_path = "Silly_Cat_Character.jpg";
    }

    int target_width  = choices["width"].as<int>();
    int target_height = choices["height"].as<int>();

    int width, height, channels;
    unsigned char* img = stbi_load(img_path.c_str(), &width, &height, &channels, 3);

    if (!img) {
        std::cerr << "Fehler beim Laden des Bildes\n";
        return 1;
    }

    constexpr float y_aspect = 2.0f;

    bool width_set  = target_width  > 0;
    const bool height_set = target_height > 0;

    // Default
    if (!width_set && !height_set) {
        target_width = 120;
        width_set = true;
    }

    float scale_x;
    float scale_y;

    // Zielgröße berechnen
    if (width_set && height_set) {
        // Stretch (explizit)
        scale_x = static_cast<float>(width)  / target_width;
        scale_y = static_cast<float>(height) / target_height;
    }
    else if (width_set) {
        scale_x = static_cast<float>(width) / target_width;
        scale_y = scale_x * y_aspect;
        target_height = static_cast<int>(height / scale_y);
    }
    else {
        scale_y = static_cast<float>(height) / target_height;
        scale_x = scale_y / y_aspect;
        target_width = static_cast<int>(width / scale_x);
    }

    // Rendering
    for (int y = 0; y < target_height; ++y) {
        for (int x = 0; x < target_width; ++x) {

            int src_x = static_cast<int>(x * scale_x);
            int src_y = static_cast<int>(y * scale_y);

            src_x = std::min(src_x, width  - 1);
            src_y = std::min(src_y, height - 1);

            const int index = (src_y * width + src_x) * 3;

            std::cout << brightness_to_ascii(
                img[index],
                img[index + 1],
                img[index + 2]
            );
        }
        std::cout << '\n';
    }

    stbi_image_free(img);
    return 0;
}
