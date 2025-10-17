//#define STB_IMAGE_WRITE_IMPLEMENTATION  // Implementation wird in stb_impl.cpp bereitgestellt
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include "../include/img_utils.h"
#include "../include/gif_utils.h"

using namespace std;
namespace fs = std::filesystem;

int main(int argc, char *argv[]) {
    try {
        // Damit UTF-8 und ANSI Farben in der Windows-Konsole funktionieren
        // compiliert auf nicht windows Systemen passiert gar nichts, ausser eine warnung in cerr zu schreiben
        enable_vt_mode();
        string ascii_chars = "@%#*+=-:. ";  // Standard-Zeichensatz
        string tmp_frames_naming_scheme;    // Derzeit nicht implementiert
        fs::path image_path;                // Pfad zum Eingabebild
        fs::path output_path;               // Ausgabe-Dateipfad wenn man --output benutzt
        fs::path tmp_dir;                   // temporäres Verzeichnis für GIF-Frames
        int width = 70;                     // Standardbreite ist 70 Zeichen
        bool colored = false;               // standardmäßig keine farbige Ausgabe
        bool gif = false;                   // standardmäßig wird nicht davon ausgegangen das der input ein GIF ist
        bool keep_frames = false;           // behalte temporäre Frames standardmäßig nicht

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
            } else if (arg == "--tmp_dir" && i + 1 < argc) {
                tmp_dir = fs::path(argv[++i]);
            } else if (arg == "-h" || arg == "--help") {
                print_help();
                return 0;
            } else if (arg == "--tmp_frames_naming_scheme" && i + 1 < argc) {
                tmp_frames_naming_scheme = argv[++i];
            } else if (arg == "--img") {
                image_path = argv[++i];
            } else {
                cerr << "Unbekanntes Argument: " << arg << endl;
            }
        }


        // --colored und --output sind nicht kompatibel
        if (colored && !output_path.empty()) {
            throw runtime_error("--colored und --output sind nicht kompatibel (Farben brauchen Terminal)");
        }

        // if tmp_dir is not provided use default path
        if (tmp_dir.empty()) {
            tmp_dir = "./tmp_gif_frames";
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

        // if tmp_frames_naming_scheme is not provided use default naming scheme
        if (tmp_frames_naming_scheme.empty()) {
            tmp_frames_naming_scheme = "/frame_%03d.png";
        }

        // Check if tmp_frames_naming_scheme contains %03d
        std::regex re("%0?\\d*d");
        if (!std::regex_search(tmp_frames_naming_scheme, re)) {
            throw std::runtime_error(
                "--tmp_frames_naming_scheme muss ein '%d'-Platzhalter enthalten (z.B. '%03d') für die Frame-Nummerierung"
            );
        }

        // Check if tmp_frames_naming_scheme ends with .png, .jpg or .jpeg
        if (tmp_frames_naming_scheme.find(".png") == string::npos &&
            tmp_frames_naming_scheme.find(".jpg") == string::npos &&
            tmp_frames_naming_scheme.find(".jpeg") == string::npos) {
            throw runtime_error("--tmp_frames_naming_scheme muss auf .png, .jpg oder .jpeg enden");
        }

        // --keep-frames only works with --gif
        if (keep_frames == true && gif == false) {
            throw runtime_error("--keep-frames funktioniert nur mit --gif");
        }

        // Check if file exists
        if (!fs::exists(image_path)) {
            throw runtime_error("Datei nicht gefunden: " + image_path.string());
        }

        cout << "Lade: " << image_path << " (Breite: " << width << ")" << endl;
        string ascii;
        if (gif == true) {
            // gif_to_ascii gibt die Animation direkt aus, also brauchen wir den Rückgabewert nicht
            gif_to_ascii(image_path.string(), width, ascii_chars, keep_frames, colored, tmp_dir, tmp_frames_naming_scheme);
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

// TODO: Extract frame delays / disposal info and save as JSON alongside frames. This will allow accurate playback timing later. (Nice to have.)
// TODO: Try to use a C++ image library to extract GIF frames directly instead of relying on ImageMagick. (Harder.)