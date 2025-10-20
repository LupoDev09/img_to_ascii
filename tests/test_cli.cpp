#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include <cstdlib>
#include <string>
#include <array>
#include <memory>
#include <iostream>

std::string run_cmd(const std::string& cmd) {
    std::array<char, 128> buffer{};
    std::string result;
    // stderr (2) -> stdout (1) umleiten
    std::unique_ptr<FILE, decltype(+pclose)> pipe(popen((cmd + " 2>&1").c_str(), "r"), +pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        result += buffer.data();
    return result;
}

// Testfälle für die CLI-Optionen
TEST_CASE("Help wird korrekt angezeigt", "[cli]") {
    std::string output = run_cmd("./img_to_ascii --help");
    REQUIRE(output.find("Usage:") != std::string::npos);
    REQUIRE(output.find("--img") != std::string::npos);
    REQUIRE(output.find("--ascii") != std::string::npos);
}

TEST_CASE("Standardwerte werden korrekt gesetzt", "[cli]") {
    std::string output = run_cmd("./img_to_ascii");
    REQUIRE(output.find("Silly_Cat_Character_.jpg") != std::string::npos);
}

TEST_CASE("Optionen werden erkannt und Fehler korrekt ausgegeben", "[cli]") {
    std::string output = run_cmd("./img_to_ascii --width 100 --fps 20 --loop 3");
    REQUIRE_FALSE(output.empty());
    REQUIRE(output.find("--fps funktioniert nur mit --gif") != std::string::npos);
}

// Check for various invalid inputs and ensure proper error messages are shown
TEST_CASE("Ungültige Eingabedatei wird korrekt behandelt", "[cli]") {
    std::string output = run_cmd("./img_to_ascii --img nonexistent_file.jpg");
    REQUIRE(output.find("Datei nicht gefunden") != std::string::npos);
}

TEST_CASE("Ungültige Breite wird korrekt behandelt", "[cli]") {
    std::string output = run_cmd("./img_to_ascii --width -50");
    REQUIRE(output.find("Breite muss größer als 0 sein") != std::string::npos);
}

TEST_CASE("Ungültige FPS wird korrekt behandelt", "[cli]") {
    std::string output = run_cmd("./img_to_ascii --gif --fps 0");
    REQUIRE(output.find("Frame-Rate muss größer als 0 sein") != std::string::npos);
}

TEST_CASE("Ungültige ASCII-Zeichenfolge wird korrekt behandelt", "[cli]") {
    std::string output = run_cmd("./img_to_ascii --ascii \"\"");
    REQUIRE(output.find("--ascii darf nicht leer sein") != std::string::npos);
}

TEST_CASE("Inkompatible Optionen werden korrekt behandelt", "[cli]") {
    std::string output = run_cmd("./img_to_ascii --colored --output ascii.txt");
    REQUIRE(output.find("--colored und --output sind nicht kompatibel") != std::string::npos);
}

TEST_CASE("Keep-Frames ohne GIF wird korrekt behandelt", "[cli]") {
    std::string output = run_cmd("./img_to_ascii --keep-frames");
    REQUIRE(output.find("--keep-frames funktioniert nur mit --gif") != std::string::npos);
}

// Zusätzliche Testfälle für temporäre Frame-Optionen
TEST_CASE("Ungültiges Benennungsschema für temporäre Frames wird korrekt behandelt", "[cli]") {
    std::string output = run_cmd("./img_to_ascii --gif --tmp-frames-naming-scheme frame_%03d.txt");
    REQUIRE(output.find("--tmp_frames_naming_scheme muss auf .png, .jpg oder .jpeg enden") != std::string::npos);
}

TEST_CASE("Ungültiges Benennungsschema ohne Platzhalter wird korrekt behandelt", "[cli]") {
    std::string output = run_cmd("./img_to_ascii --gif --tmp-frames-naming-scheme frame.png");
    REQUIRE(output.find("--tmp_frames_naming_scheme muss ein '%d'-Platzhalter enthalten") != std::string::npos);
}

// Zusätzliche Testfälle für Farbausgabe
TEST_CASE("Farbausgabe mit ungültigem Dateityp wird korrekt behandelt", "[cli]") {
    std::string output = run_cmd("./img_to_ascii --colored --output ascii.txt");
    REQUIRE(output.find("--colored und --output sind nicht kompatibel") != std::string::npos);
}

// Zusätzliche Testfälle für GIF-Optionen
TEST_CASE("Loop-Option mit ungültigem Wert wird korrekt behandelt", "[cli]") {
    std::string output = run_cmd("./img_to_ascii --gif --loop -1");
    REQUIRE(output.find("Loop-Wert muss größer oder gleich 0 sein") != std::string::npos);
}

// Zusätzliche Testfälle für Ausgabeoptionen
TEST_CASE("Ungültiger Ausgabepfad wird korrekt behandelt", "[cli]") {
    std::string output = run_cmd("./img_to_ascii --output /invalid_path/ascii.txt");
    REQUIRE(output.find("Konnte Datei nicht öffnen: ") != std::string::npos);
}

TEST_CASE("Keine Ausgabeoptionen angegeben", "[cli]") {
    std::string output = run_cmd("./img_to_ascii --img Silly_Cat_Character_.jpg --width 80");
    REQUIRE(output.find("ASCII-Art") != std::string::npos);
}

// Zusätzliche Testfälle für Bildoptionen
TEST_CASE("Ungültiger Bildpfad wird korrekt behandelt", "[cli]") {
    std::string output = run_cmd("./img_to_ascii --img invalid_image.jpg");
    REQUIRE(output.find("Datei nicht gefunden") != std::string::npos);
}

TEST_CASE("GIF-Option ohne Bilddatei wird korrekt behandelt", "[cli]") {
    std::string output = run_cmd("./img_to_ascii --gif");
    REQUIRE(output.find("GIF konnte nicht geladen werden: not GIF") != std::string::npos);
}

// Zusätzliche Testfälle für Breitenoptionen
TEST_CASE("Breite als nicht-numerischer Wert wird korrekt behandelt", "[cli]") {
    std::string output = run_cmd("./img_to_ascii --width abc");
    REQUIRE(output.find("Argument") != std::string::npos);
    REQUIRE(output.find("failed to parse") != std::string::npos);
}

TEST_CASE("Bei Großer breite eine warnung ausgeben", "[cli]") {
    std::string output = run_cmd("./img_to_ascii --width 600");
    REQUIRE(output.find("Warnung: Eine sehr große Breite kann die Anzeige in der Konsole beeinträchtigen.") != std::string::npos);
}

TEST_CASE("Sehr große Breite wird korrekt behandelt", "[cli]") {
    std::string output = run_cmd("./img_to_ascii --width 10000");
    REQUIRE(output.find("Breite zu groß! Bitte einen Wert unter 1000 wählen.") != std::string::npos);
}

// Zusätzliche Testfälle für FPS-Optionen
TEST_CASE("FPS als nicht-numerischer Wert wird korrekt behandelt", "[cli]") {
    std::string output = run_cmd("./img_to_ascii --gif --fps abc");
    REQUIRE(output.find("Argument") != std::string::npos);
    REQUIRE(output.find("failed to parse") != std::string::npos);
}

// Zusätzliche Testfälle für Loop-Optionen
TEST_CASE("Loop als nicht-numerischer Wert wird korrekt behandelt", "[cli]") {
    std::string output = run_cmd("./img_to_ascii --gif --loop abc");
    REQUIRE(output.find("Argument") != std::string::npos);
    REQUIRE(output.find("failed to parse") != std::string::npos);
}