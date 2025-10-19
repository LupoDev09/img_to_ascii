//
// Created by lupo on 17.10.25.
//

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <filesystem>
#include "../include/img_utils.h"
#include "../include/stb_image_write.h"
#include "../include/stb_image.h"
#include "thread"

using namespace std;
namespace fs = std::filesystem;

/**
 *@brief Decodes a GIF file into individual PNG frames saved in the specified output directory.
 *
 *@param gif_path Path to the input GIF file.
 *@param out_dir Directory where extracted PNG frames will be saved.
 *@param tmp_frames_naming_scheme Naming scheme for temporary GIF frames (e.g., "frame_%03d.png").
 *
 *@throws std::runtime_error If the GIF cannot be loaded or if file operations fail.
*/
void decode_gif_to_frames(const std::string &gif_path, const fs::path &out_dir, const std::string& tmp_frames_naming_scheme) {
    int width, height, frames, channels;
    int *delays = nullptr;

    // Datei einlesen
    ifstream file(gif_path, ios::binary | ios::ate);
    if (!file.is_open())
        throw runtime_error("Konnte Datei nicht öffnen: " + gif_path);
    streamsize size = file.tellg();
    file.seekg(0, ios::beg);
    vector<unsigned char> buffer(static_cast<unsigned long>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
        throw runtime_error("Fehler beim Lesen der Datei");

    // stb_image liefert flachen RGBA-Buffer für alle Frames
    unsigned char *frames_data = stbi_load_gif_from_memory(
        buffer.data(), static_cast<int>(size),
        &delays, &width, &height, &frames, &channels, 4
    );

    if (!frames_data) // Fehler beim Laden des GIFs
        throw runtime_error("GIF konnte nicht geladen werden: " + string(stbi_failure_reason()));

    fs::create_directories(out_dir); // Erstelle Ausgabeverzeichnis falls nicht existent

    size_t frame_size = static_cast<size_t>(width * height * 4); // RGBA
    for (int i = 0; i < frames; ++i) {
        // Zeiger auf den aktuellen Frame im flachen Buffer
        const unsigned char *frame_ptr = frames_data + (static_cast<size_t>(i) * frame_size);

        char filename[128];// Puffer für Dateinamen
        // Erstelle Dateinamen basierend auf dem Benennungsschema
        snprintf(filename, sizeof(filename), tmp_frames_naming_scheme.c_str(), i);
        string frame_path = (out_dir / filename).string();

        // Schreibe den Frame als PNG
        stbi_write_png(frame_path.c_str(), width, height, 4, frame_ptr, width * 4);
        // Debug info
        cout << "Gespeichert: " << frame_path << " (Delay: " << delays[i] << "ms)\n";
    }

    stbi_image_free(frames_data);
    delete[] delays;
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
// TODO: Vielleicht in kleinere funktionen aufteilen.
// TODO: Später Frame-Metadaten (Delays) extrahieren und als JSON speichern.
// TODO: Eventuell Multithreading für die Frame-Verarbeitung implementieren.
void gif_to_ascii(const std::string &gif_path, const int width, const std::string &ascii_chars,const bool keep_tmp, const bool colored,
    const std::filesystem::path &out_dir, const std::string& tmp_frames_naming_scheme, const int fps) {

    const fs::path input_gif = gif_path;    // GIF-pfad als filesystem path übersetzen
    const int delay_ms = 1000 / fps;        // Verzögerung zwischen Frames in Millisekunden

    // Überprüfe ob die Eingabedatei existiert
    if (!fs::exists(input_gif)) {
        cerr << "Input file does not exist: " << input_gif << "\n";
        return ;
    }

    // Extrahiere Frames aus dem GIF
    decode_gif_to_frames(input_gif, out_dir, tmp_frames_naming_scheme);

    cout << "\033[?25l";
    // Erzeuge ASCII für jeden Frame
    if (!colored) {
        // Gehe durch alle extrahierten Frames im Verzeichnis
        for (auto &f : fs::directory_iterator(out_dir.string())) {
            cout << "\033[H";                                                   // Cursor an den Anfang setzen
            string ascii = image_to_ascii(f.path().string());           // frame in ascii umwandeln
            cout << ascii << endl;                                              // Ausgabe des ASCII
            this_thread::sleep_for(chrono::milliseconds(delay_ms));        // Warte für die Frame-Rate
        }
    } else {
        for (auto &f : fs::directory_iterator(out_dir.string())) {
            cout << "\033[H";                                                                   // Cursor an den Anfang setzen
            string ascii = image_to_ascii_color(f.path().string(), width, ascii_chars); // frame in ascii umwandeln
            cout << ascii << endl;                                                              // Ausgabe des ASCII
            this_thread::sleep_for(chrono::milliseconds(delay_ms));                        // Warte für die Frame-Rate
        }
    }
    cout << "\033[?25h";    // Zeige den Cursor wieder
    // Entferne temporäres Verzeichnis falls nicht behalten
    if (!keep_tmp) {
        try {
            fs::remove_all(out_dir);
        } catch (const exception &e) {
            cerr << "Warnung: Konnte temporäre Frames nicht löschen: " << e.what() << "\n";
        }
    } else {
        cout << "Frames behalten in: " << out_dir << "\n";
    }

}
