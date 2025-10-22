//
// Created by lupo on 21.10.25.
//
// Check for various invalid inputs and ensure proper error messages are shown
#include "test_utils.h"
#include <catch2/catch_all.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

TEST_CASE("Ungültige Eingabedatei wird korrekt behandelt", "[cli]") {
  std::string output = run_cmd("./img_to_ascii --img nonexistent_file.jpg");
  REQUIRE(output.find("Bild nicht gefunden") != std::string::npos);
}

TEST_CASE("Ungültige Breite wird korrekt behandelt", "[cli]") {
  std::string output = run_cmd("./img_to_ascii --width -50");
  REQUIRE(output.find("Breite muss größer als 0 sein") != std::string::npos);
}

TEST_CASE("Ungültige FPS wird korrekt behandelt", "[cli]") {
  std::string output = run_cmd("./img_to_ascii --gif --fps 0");
  REQUIRE(output.find("Frame-Rate muss größer als 0 sein") !=
          std::string::npos);
}

TEST_CASE("Ungültige ASCII-Zeichenfolge wird korrekt behandelt", "[cli]") {
  std::string output = run_cmd("./img_to_ascii --ascii \"\"");
  REQUIRE(output.find("--ascii darf nicht leer sein") != std::string::npos);
}

TEST_CASE("Inkompatible Optionen werden korrekt behandelt", "[cli]") {
  std::string output = run_cmd("./img_to_ascii --colored --output ascii.txt");
  REQUIRE(output.find("--colored und --output sind nicht kompatibel") !=
          std::string::npos);
}

TEST_CASE("Keep-Frames ohne GIF wird korrekt behandelt", "[cli]") {
  std::string output = run_cmd("./img_to_ascii --keep-frames");
  REQUIRE(output.find("--keep-frames funktioniert nur mit --gif") !=
          std::string::npos);
}