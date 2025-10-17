//
// Created by lupo on 17.10.25.
//

#ifndef IMG_TO_ASCII_GIF_UTILS_H
#define IMG_TO_ASCII_GIF_UTILS_H
#include <filesystem>
#include <string>
#include <vector>

// Extrahiere Frames aus einer GIF-Datei in ein Ausgabeverzeichnis.
// Rückgabe: sortierte Liste der erzeugten PNG-Dateien (vollständige Frames).
// Wenn ImageMagick verfügbar ist, wird "magick ... -coalesce -alpha set PNG32:..." verwendet,
// damit die resultierenden PNGs RGBA (kein palettiertes/graues PNG) sind.
// Falls ImageMagick nicht vorhanden ist, wird als Fallback nur die erste Frame via stb_image gespeichert.
static std::vector<std::filesystem::path> extract_frames(const std::filesystem::path &input_gif, const std::filesystem::path &out_dir);

// Wandelt ein GIF in ASCII um, indem es alle extrahierten PNG-Frames der Reihe nach
// mit image_to_ascii() verarbeitet. Wenn keep_tmp == false, werden die temporären
// Frames nach der Verarbeitung gelöscht (Standardverhalten).
std::string gif_to_ascii(const std::string &gif_path, int width, const std::string &ascii_chars, bool keep_tmp = false);
#endif //IMG_TO_ASCII_GIF_UTILS_H
