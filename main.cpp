#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

// stb_image (Header-Only Image Loader) → https://github.com/nothings/stb
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;
namespace fs = filesystem;

/**
 * Bild zu ASCII-Art
 *
 * @param filename
 * @param output_width
 * @param ascii_chars
 * @return img as ascii string
 */
string image_to_ascii(const string &filename, int output_width = 70, const string &ascii_chars = "@%#*+=-:. ") {
    int width, height, channels;
    unsigned char *img = stbi_load(filename.c_str(), &width, &height, &channels, 0);
    if (!img) {
        throw runtime_error("Fehler: Bild konnte nicht geladen werden!");
    }

    if (channels < 3) {
        stbi_image_free(img);
        throw runtime_error("Bild hat zu wenige Farbkanäle (mind. RGB nötig)");
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

            // RGB-Werte holen
            unsigned char r = img[idx + 0];
            unsigned char g = img[idx + 1];
            unsigned char b = img[idx + 2];

            // Transparenz prüfen, falls Alpha-Kanal vorhanden
            if (channels >= 4) {
                unsigned char a = img[idx + 3];
                if (a < 128) { // Pixel halbtransparent oder unsichtbar → Leerzeichen
                    ascii.push_back(' ');
                    continue;
                }
            }

            // Grauwert berechnen (RGB → Luminanz)
            auto gray = static_cast<unsigned char>(0.299 * r + 0.587 * g + 0.114 * b);

            // Mapping: Grau 0-255 auf ascii_chars
            int char_index = gray * (ascii_chars.size() - 1) / 255;
            ascii.push_back(ascii_chars[char_index]);
        }
        ascii += '\n';
    }

    stbi_image_free(img);
    return ascii;
}

/**
 * Bild zu ASCII-Art aber mit Farbe
 *
 * @param filename
 * @param output_width
 * @param ascii_chars
 * @return img as ascii string
 */
string image_to_ascii_color(const string &filename, int output_width = 70, const string &ascii_chars = "@%#*+=-:. ") {
    int width, height, channels;
    unsigned char *img = stbi_load(filename.c_str(), &width, &height, &channels, 0);
    if (!img) {
        throw runtime_error("Fehler: Bild konnte nicht geladen werden!");
    }

    if (channels < 3) {
        stbi_image_free(img);
        throw runtime_error("Bild hat zu wenige Farbkanäle (mind. RGB nötig)");
    }

    float aspect_ratio = static_cast<float>(height) / static_cast<float>(width);
    int output_height = static_cast<int>(output_width * aspect_ratio * 0.55f);

    string ascii;
    ascii.reserve(output_width * output_height * 30); // Platz für ANSI-Codes

    float x_step = static_cast<float>(width) / output_width;
    float y_step = static_cast<float>(height) / output_height;

    for (int y = 0; y < output_height; y++) {
        for (int x = 0; x < output_width; x++) {
            int px = static_cast<int>(x * x_step);
            int py = static_cast<int>(y * y_step);

            int idx = (py * width + px) * channels;

            unsigned char r = img[idx + 0];
            unsigned char g = img[idx + 1];
            unsigned char b = img[idx + 2];

            // Transparenz prüfen
            if (channels >= 4) {
                unsigned char a = img[idx + 3];
                if (a < 128) {
                    ascii += " ";
                    continue;
                }
            }

            // Grauwert → Zeichen auswählen
            unsigned char gray = static_cast<unsigned char>(0.299 * r + 0.587 * g + 0.114 * b);
            int char_index = gray * (ascii_chars.size() - 1) / 255;
            char c = ascii_chars[char_index];

            // ANSI 24-Bit Farbcodes (Vordergrundfarbe)
            ascii += "\033[38;2;" + to_string(r) + ";" + to_string(g) + ";" + to_string(b) + "m";
            ascii.push_back(c);
        }
        ascii += "\033[0m\n"; // Reset am Zeilenende
    }

    stbi_image_free(img);
    return ascii;
}

/**
 *Gibt die Hilfe auf der Konse aus
 */
void print_help() {
    cout << "Usage: \n"
         << "img_to_ascii [--help || -h] [Path_to_img] [-w width] [--ascii] [-o || --output output_file] [--colored]\n"
         << "--help/-h \t prints this help\n"
         << "Path_to_img \t is the path to the used img\n"
         << "-w/--width \t is the width of the image\n"
         << "--ascii \t configure the chars used for the art\n"
         << "-o/--output \t output file\n"
         << "--colored \t add some colors in the ascii\n"
         << "\nNote:\n\t --colored is not compatable with --output and -o\n"
         << "\tThis thing does not work on Windof"
         << endl;
}

int main(int argc, char *argv[]) {
    try {
        string ascii_chars = "@%#*+=-:. ";
        fs::path image_path;
        fs::path output_path;
        int width = 70; // Default
        bool colored = false;

        for (int i = 1; i < argc; i++) {
            string arg = argv[i];
            if ((arg == "-w" || arg == "--width") && i + 1 < argc) {
                // Get width parameter
                try {
                    width = stoi(argv[++i]);
                } catch (...) {
                    throw runtime_error("Ungültige Breite bei -w");
                }
            } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
                output_path = fs::path(argv[++i]);
            } else if (arg == "--ascii" && i + 1 < argc) {
                ascii_chars = argv[++i];
            }else if (arg == "--colored") {
                colored = true;
            } else if (arg == "-h" || arg == "--help") {
                print_help();
                return 0;
            } else { // Get img path
                image_path = arg;
            }
        }

        if (colored && !output_path.empty()) {
            throw runtime_error("--colored und --output sind nicht kompatibel (Farben brauchen Terminal)");
        }

        // if path is not provided use default path
        if (image_path.empty()) {
            fs::path exe_path = fs::absolute(argv[0]);
            fs::path exe_dir = exe_path.parent_path();
            image_path = exe_dir / "Silly_Cat_Character_.jpg";
        }

        if (ascii_chars.empty()) {
            throw runtime_error("--ascii darf nicht leer sein! entweder lass es weg oder gib es einen wert");
        }

        cout << "Lade: " << image_path << " (Breite: " << width << ")" << endl;
        string ascii;
        if (colored) {
            ascii = image_to_ascii_color(image_path.string(), width, ascii_chars);
        } else {
            ascii = image_to_ascii(image_path.string(), width, ascii_chars);
        }

        if (output_path.empty()) {
            cout << ascii << endl;
        } else {
            ofstream out(output_path);
            if (!out) {
                throw runtime_error("Konnte Datei nicht zum Schreiben öffnen: " + output_path.string());
            }
            out << ascii;
            cout << "ASCII-Art in Datei geschrieben: " << output_path << endl;
        }

    } catch (const exception& error) {
        cerr << error.what() << "\n";
        print_help();
    }
    cout << "\033[0m"; // Reset ganz am Ende von main
    return 0;
}
