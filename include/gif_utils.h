//
// Created by lupo on 17.10.25.
//

#ifndef IMG_TO_ASCII_GIF_UTILS_H
#define IMG_TO_ASCII_GIF_UTILS_H
#include <filesystem>
#include <string>

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
 *@param fps Frame-Rate für die Anzeige der ASCII-Animation (in Frames pro Sekunde)
 *@return ASCII-Animation als String
*/

void gif_to_ascii(
    const std::string &gif_path,
    int width,
    const std::string &ascii_chars,
    bool keep_tmp = false,
    bool colored = false,
    const std::filesystem::path &out_dir = "./tmp_gif_frames",
    const std::string& tmp_frames_naming_scheme = "frame_%03d.png",
    int fps = 1,
    const bool write_json = false);
#endif //IMG_TO_ASCII_GIF_UTILS_H
