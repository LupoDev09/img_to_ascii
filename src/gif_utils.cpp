//
// Created by lupo on 17.10.25.
//

#include <filesystem>
#include <iostream>
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

/**
 * @brief Extrahiere Frames aus einer GIF-Datei in ein Ausgabeverzeichnis.
 *
 * @details Wenn ImageMagick verfügbar ist, wird "magick ... -coalesce -alpha set PNG32:..." verwendet,
 * damit die resultierenden PNGs RGBA (kein palettiertes/graues PNG) sind.
 * Falls ImageMagick nicht vorhanden ist, wird als Fallback nur die erste Frame via stb_image gespeichert.
 *
 * @param input_gif Pfad zur Eingabe-GIF-Datei
 * @param out_dir Pfad zum Ausgabeverzeichnis für die extrahierten Frames
 * @param tmp_frames_naming_scheme Benennungsschema für temporäre GIF-Frames
 *
 * @return Sortierter Vektor mit Pfaden zu den extrahierten PNG-Frames
*/
vector<fs::path> extract_frames(const fs::path &input_gif, const fs::path &out_dir, const string& tmp_frames_naming_scheme) {
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
    const string cmd = "magick \"" + input_gif.string() + "\" -coalesce -alpha set PNG32:\"" + out_dir.string() + "/" + tmp_frames_naming_scheme + "\"";
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
        frames.push_back(out);
    } else {
        cout << "Frames erfolgreich extrahiert nach: " << out_dir << "\n";
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
    ranges::sort(frames);
    return frames;
}


/**
 *@brief Wandelt ein GIF in ASCII um, indem es alle extrahierten PNG-Frames der Reihe nach
 *mit image_to_ascii() verarbeitet. Wenn keep_tmp == false, werden die temporären
 *Frames nach der Verarbeitung gelöscht (Standardverhalten).
 *
 *@param gif_path Pfad zur Eingabe-GIF-Datei
 *@param width Gewünschte Breite des ASCII-Ausgabe (in ASCII-Zeichen)
 *@param ascii_chars Zeichen, die für die ASCII-Darstellung verwendet werden sollen
 *@param keep_tmp Ob die temporären extrahierten Frames beibehalten werden sollen (standardmäßig false)
 *@param colored Ob die ASCII-Ausgabe in Farbe erfolgen soll
 *@param out_dir Verzeichnis zum Speichern der temporären Frames
 *@param tmp_frames_naming_scheme Benennungsschema für temporäre GIF-Frames
 *@param fps Bildwiederholrate für die Animation (Frames pro Sekunde)
 *
 *@return ASCII-Animation als String
*/
// TODO: Später Frame-Metadaten (Delays) extrahieren und als JSON speichern.
void gif_to_ascii(const std::string &gif_path, int width, const std::string &ascii_chars, bool keep_tmp, bool colored,
    std::filesystem::path out_dir, const std::string& tmp_frames_naming_scheme, int fps) {
    const fs::path input_gif = gif_path;
    int delay_ms = 1000 / fps;

    if (!fs::exists(input_gif)) {
        cerr << "Input file does not exist: " << input_gif << "\n";
        return ;
    }

    // Extrahiere Frames (ImageMagick oder Fallback)
    const vector<fs::path> frames = extract_frames(input_gif, out_dir, tmp_frames_naming_scheme);
    if (frames.empty()) {
        cerr << "Keine Frames gefunden oder Fehler bei der Extraktion.\n";
        return ;
    }

    std::cout << "\033[?25l";
    // Erzeuge ASCII für jeden Frame
    if (!colored) {
        for (size_t i = 0; i < frames.size(); ++i) {
            const auto &p = frames[i];
            try {
                string ascii_frame = image_to_ascii(p.string(), width, ascii_chars);
                cout << ascii_frame;
                if (i + 1 != frames.size()) {
                    cout << "\033[H";
                }
                this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            } catch (const std::exception &e) {
                cerr << "Warnung: Fehler beim Verarbeiten von " << p << ": " << e.what() << "\n";
            }
        }
    } else {
        for (size_t i = 0; i < frames.size(); ++i) {
            const auto &p = frames[i];
            try {
                string ascii_frame = image_to_ascii_color(p.string(), width, ascii_chars);
                cout << ascii_frame;
                if (i + 1 != frames.size()) {
                    cout << "\033[H";
                }
                this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            } catch (const std::exception &e) {
                cerr << "Warnung: Fehler beim Verarbeiten von " << p << ": " << e.what() << "\n";
            }
        }
    }
    std::cout << "\033[?25h";
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

}
