#include <iostream>
#include <string>
#include <cxxopts.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// ASCII-Zeichen von dunkel → hell
const std::string ASCII = "@%#*+=-:. ";

char brightness_to_ascii(const unsigned char r, const unsigned char g, const unsigned char b) {
    // Wahrnehmungs-korrekte Helligkeit
    const size_t brightness = 0.2126f * r + 0.7152f * g + 0.0722f * b;
    const size_t index = (brightness / 255.0f) * (ASCII.size() - 1);
    return ASCII[index];
}

int main(int argc, char** argv) {
    cxxopts::Options options("img_to_ascii", "My try to rewrite my img to ascii tool");
    options.add_options()
    ("h,help", "produce help message")
    ("img", "The image to load", cxxopts::value<std::string>()->default_value("Silly_Cat_Character.jpg"));

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

    int width, height, channels;
    unsigned char* img = stbi_load(img_path.c_str(), &width, &height, &channels, 3);

    if (!img) {
        std::cerr << "Fehler beim Laden des Bildes\n";
        return 1;
    }

    // Terminal-Zeichen sind höher als breit → Y-Skalierung
    constexpr int y_step = 2;

    for (int y = 0; y < height; y += y_step) {
        for (int x = 0; x < width; x++) {
            const int index = (y * width + x) * 3;
            const unsigned char r = img[index];
            const unsigned char g = img[index + 1];
            const unsigned char b = img[index + 2];

            std::cout << brightness_to_ascii(r, g, b);
        }
        std::cout << '\n';
    }

    stbi_image_free(img);
    return 0;
}
