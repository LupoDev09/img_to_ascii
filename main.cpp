#include <filesystem>
#include <iostream>
#include <string>

// stb_image (Header-Only Image Loader) → https://github.com/nothings/stb
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;
// ASCII-Zeichen nach Helligkeit sortiert (dunkel → hell)
const string ASCII_CHARS = "@%#*+=-:. ";

// Funktion: Bild zu ASCII-Art
string image_to_ascii(const string& filename, int output_width = 80) {
    int width, height, channels;
    unsigned char* img = stbi_load(filename.c_str(), &width, &height, &channels, 0);
    if (!img) {
        throw runtime_error("Fehler: Bild konnte nicht geladen werden!");
    }

    // Zielhöhe proportional skalieren
    float aspect_ratio = static_cast<float>(height) / static_cast<float>(width);
    int output_height = static_cast<int>(output_width * aspect_ratio * 0.55f);
    // 0.55 für Konsolen-Zeichenhöhe korrigiert

    string ascii;
    ascii.reserve(output_width * output_height + output_height);

    // Schrittgrößen fürs Sampling (Skalierung)
    float x_step = static_cast<float>(width) / output_width;
    float y_step = static_cast<float>(height) / output_height;

    for (int y = 0; y < output_height; y++) {
        for (int x = 0; x < output_width; x++) {
            int px = static_cast<int>(x * x_step);
            int py = static_cast<int>(y * y_step);

            int idx = (py * width + px) * channels;

            // Grauwert berechnen (RGB → Luminanz)
            unsigned char r = img[idx + 0];
            unsigned char g = img[idx + 1];
            unsigned char b = img[idx + 2];
            unsigned char gray = static_cast<unsigned char>(0.299*r + 0.587*g + 0.114*b);

            // Mapping: Grau 0-255 auf ASCII_CHARS
            int char_index = (gray * (ASCII_CHARS.size() - 1)) / 255;
            ascii += ASCII_CHARS[char_index];
        }
        ascii += '\n';
    }

    stbi_image_free(img);
    return ascii;
}

void print_help() {
    cout << "Usage: img_to_ascii [Path_to_img]" << endl;
}

// @TODO: Füge ein Argument hinzu um den Path zu dem bild anzugeben
int main(int argc, char* argv[]) {
    try {
        // Verzeichnis des Executables herausfinden
        filesystem::path exe_path = filesystem::absolute(argv[0]);
        filesystem::path exe_dir = exe_path.parent_path();

        // Bild relativ zu exe_dir laden
        filesystem::path image_path = exe_dir / "Silly_Cat__Character_.jpg";

        cout << "Lade: " << image_path << endl;

        string ascii = image_to_ascii(image_path.string(), 70);
        cout << ascii << endl;

    } catch (const exception& error) {
        cerr << error.what() << "\n";
    }
    return 0;
}
