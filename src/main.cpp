//#define STB_IMAGE_WRITE_IMPLEMENTATION  // Implementation wird in stb_impl.cpp bereitgestellt
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include "../include/img_utils.h"
#include "../include/stb_image.h"
#include "../include/gif_utils.h"

using namespace std;
namespace fs = std::filesystem;

int main(int argc, char *argv[]) {
    try {
#if defined(_WIN32)
        // Damit UTF-8 und ANSI Farben in der Windows-Konsole funktionieren:
        _setmode(_fileno(stdout), _O_TEXT);
        enable_vt_mode();
#endif
        string ascii_chars = "@%#*+=-:. ";
        fs::path image_path;
        fs::path output_path;
        int width = 70;// Default
        bool colored = false;
        bool gif = false;
        bool keep_frames = false; // neue Flag: behalte tmp-Frames, wenn true

        // Parse flags
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
            } else if (arg == "--colored") {
                colored = true;
            } else if (arg == "--gif") {
                gif = true;
            } else if (arg == "--keep-frames") {
                keep_frames = true;
            } else if (arg == "-h" || arg == "--help") {
                print_help();
                return 0;
            } else {// Get img path
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

        // if ascii_chars is provided but is empty throw an error
        if (ascii_chars.empty()) {
            throw runtime_error("--ascii darf nicht leer sein! entweder lass es weg oder gib es einen wert");
        }

        // Check if file exists
        if (!fs::exists(image_path)) {
            throw runtime_error("Datei nicht gefunden: " + image_path.string());
        }

        cout << "Lade: " << image_path << " (Breite: " << width << ")" << endl;
        string ascii;
        if (gif == true) {
            // gif_to_ascii jetzt mit keep_frames-Option
            gif_to_ascii(image_path.string(), width, ascii_chars, keep_frames);
            ascii = ""; // gif_to_ascii gibt die Animation direkt aus
        } else if (colored) {
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

    } catch (const exception &error) {
        cerr << error.what() << "\n";
        print_help();
        cout << "\033[0m" << endl;// Reset ganz am Ende von main
        return 1;
    }
    cout << "\033[0m" << endl; // Reset ganz am Ende von main
    return 0;
}

// TODO: Extract frame delays / disposal info and save as JSON alongside frames.
// This will allow accurate playback timing later. (Nicht dringend für jetzt.)
// TODO: Extract those funktions out to a separate gif_utils.cpp/hpp file.
