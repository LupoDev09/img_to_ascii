//
// Created by lupo on 17.10.25.
//

#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#include "../include/img_utils.h"
#include "../include/stb_image_write.h"
#include "../include/stb_image.h"
#include "thread"

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
string gif_to_ascii(const string &gif_path, int width, const string &ascii_chars, bool keep_tmp) {
    fs::path input_gif = gif_path;
    fs::path out_dir = "./tmp_gif_frames"; // temporäres Verzeichnis

    if (!fs::exists(input_gif)) {
        cerr << "Input file does not exist: " << input_gif << "\n";
        return "";
    }

    // Extrahiere Frames (ImageMagick oder Fallback)
    vector<fs::path> frames = extract_frames(input_gif, out_dir);
    if (frames.empty()) {
        cerr << "Keine Frames gefunden oder Fehler bei der Extraktion.\n";
        return "";
    }

    // Erzeuge ASCII für jeden Frame
    string ascii_animation;
    for (size_t i = 0; i < frames.size(); ++i) {
        const auto &p = frames[i];
        try {
            string ascii_frame = image_to_ascii(p.string(), width, ascii_chars);
            cout << ascii_frame;
            if (i + 1 != frames.size()) {
                cout << "\033[H";
            }
            this_thread::sleep_for(std::chrono::milliseconds(100));
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
