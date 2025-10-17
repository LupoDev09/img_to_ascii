#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include "utils.h" // print_help, image_to_ascii, image_to_ascii_color
//#define STB_IMAGE_WRITE_IMPLEMENTATION  // Implementation wird in utils.cpp bereitgestellt
#include "stb_image_write.h"
#include "stb_image.h"

using namespace std;
namespace fs = std::filesystem;

// Extrahiere Frames aus einer GIF-Datei in ein Ausgabeverzeichnis.
// Rückgabe: sortierte Liste der erzeugten PNG-Dateien (vollständige Frames).
// Wenn ImageMagick verfügbar ist, wird "magick ... -coalesce -alpha set PNG32:..." verwendet,
// damit die resultierenden PNGs RGBA (kein palettiertes/graues PNG) sind.
// Falls ImageMagick nicht vorhanden ist, wird als Fallback nur die erste Frame via stb_image gespeichert.
static vector<fs::path> extract_frames(const fs::path &input_gif, const fs::path &out_dir) {
    vector<fs::path> frames;

    // Entferne altes temporäres Verzeichnis, damit keine alten Frames vorhanden sind
    try {
        if (fs::exists(out_dir)) {
            fs::remove_all(out_dir);
        }
    } catch (const std::exception &e) {
        cerr << "Warnung: Konnte temporäres Verzeichnis nicht löschen: " << e.what() << "\n";
    }

    // Erstelle Ausgabeverzeichnis
    try {
        fs::create_directories(out_dir);
    } catch (const std::exception &e) {
        cerr << "Fehler: Konnte Ausgabeverzeichnis nicht erstellen: " << e.what() << "\n";
        return frames;
    }

    // ImageMagick-Aufruf: -coalesce sorgt für vollständige (composited) Frames,
    // PNG32: erzwingt RGBA-Ausgabe (vermeidet palettierte/greyscale PNGs)
    string cmd = "magick \"" + input_gif.string() + "\" -coalesce -alpha set PNG32:\"" + out_dir.string() + "/frame_%03d.png\"";
    int rc = system(cmd.c_str());
    if (rc != 0) {
        // Fallback: nur erste Frame via stb_image
        cout << "ImageMagick fehlgeschlagen oder nicht installiert; speichere als Fallback nur die erste Frame via stb_image.\n";
        int w,h,n;
        unsigned char* data = stbi_load(input_gif.string().c_str(), &w, &h, &n, 4); // force RGBA
        if (!data) {
            cerr << "stbi_load failed: " << stbi_failure_reason() << "\n";
            return frames;
        }
        fs::path out = out_dir / "frame_000.png";
        if (!stbi_write_png(out.string().c_str(), w, h, 4, data, w * 4)) {
            cerr << "stbi_write_png failed\n";
            stbi_image_free(data);
            return frames;
        }
        stbi_image_free(data);
    }

    // Sammle nur PNG-Dateien und sortiere sie
    try {
        for (const auto &entry : fs::directory_iterator(out_dir)) {
            if (!entry.is_regular_file()) continue;
            auto ext = entry.path().extension().string();
            for (auto &c : ext) c = static_cast<char>(std::tolower(c));
            if (ext == ".png") frames.push_back(entry.path());
        }
    } catch (const std::exception &e) {
        cerr << "Fehler beim Lesen des Frame-Verzeichnisses: " << e.what() << "\n";
        return frames;
    }
    sort(frames.begin(), frames.end());
    return frames;
}

// Wandelt ein GIF in ASCII um, indem es alle extrahierten PNG-Frames der Reihe nach
// mit image_to_ascii() verarbeitet. Wenn keep_tmp == false, werden die temporären
// Frames nach der Verarbeitung gelöscht (Standardverhalten).
// TODO: Später Frame-Metadaten (Delays) extrahieren und als JSON speichern.
string gif_to_ascii(const string &gif_path, int width, const string &ascii_chars, bool keep_tmp = false) {
    fs::path input_gif = gif_path;
    fs::path out_dir = "./tmp_gif_frames"; // temporäres Verzeichnis

    if (!fs::exists(input_gif)) {
        cerr << "Input file does not exist: " << input_gif << "\n";
        return "";
    }

    // Extrahiere Frames (ImageMagick oder Fallback)
    auto frames = extract_frames(input_gif, out_dir);
    if (frames.empty()) {
        cerr << "Keine Frames gefunden oder Fehler bei der Extraktion.\n";
        return "";
    }

    // Erzeuge ASCII für jede Frame
    string ascii_animation;
    for (const auto &p : frames) {
        try {
            string ascii_frame = image_to_ascii(p.string(), width, ascii_chars);
            ascii_animation += ascii_frame;
            // Cursor-Reset, damit die Animation später im Terminal klappt
            ascii_animation += "\033[H";
        } catch (const std::exception &e) {
            cerr << "Warnung: Fehler beim Verarbeiten von " << p << ": " << e.what() << "\n";
        }
    }

    // Entferne temporäres Verzeichnis falls nicht behalten
    if (!keep_tmp) {
        try {
            fs::remove_all(out_dir);
        } catch (const std::exception &e) {
            cerr << "Warnung: Konnte temporäre Frames nicht löschen: " << e.what() << "\n";
        }
    } else {
        cout << "Frames behalten in: " << out_dir << "\n";
    }

    return ascii_animation;
}

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
            ascii = gif_to_ascii(image_path.string(), width, ascii_chars, keep_frames);
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
