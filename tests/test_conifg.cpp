//
// Created by lupo on 21.10.25.
//

#include <catch2/catch_all.hpp>
#include <cstdlib>
#include <string>
#include <memory>
#include <iostream>
#include "test_utils.h"

TEST_CASE("Konfigurationsdatei wird korrekt geladen", "[config]") {
    std::string output = run_cmd("./img_to_ascii --load-config  ../example_config.json");
    REQUIRE(output.find("Konfigurationsdatei geladen") != std::string::npos);
    REQUIRE(output.find("Bildpfad: \"funny.gif\"") != std::string::npos);
    REQUIRE(output.find("Breite: 100") != std::string::npos);
    REQUIRE(output.find("ASCII-Zeichen: @%#*+=-:. ") != std::string::npos);
    REQUIRE(output.find("Farbig: Ja") != std::string::npos);
    REQUIRE(output.find("GIF-Modus: Ja") != std::string::npos);
    REQUIRE(output.find("Frame-Rate: 15") != std::string::npos);
    REQUIRE(output.find("Loop: 1") != std::string::npos);
}

TEST_CASE("Ungültiger Pfad zur Konfigurationsdatei wird korrekt behandelt", "[config]") {
    std::string output = run_cmd("./img_to_ascii --load-config invalid_config.json");
    REQUIRE(output.find("Config nicht gefunden") != std::string::npos);
}

TEST_CASE("Konfigurationsdatei mit ungültiger endung", "[config]") {
    std::string output = run_cmd("./img_to_ascii --load-config ../invalid_config.mit_ausgedachter_endung");
    REQUIRE(output.find("Config nicht gefunden") != std::string::npos);
}
TEST_CASE("Kommandozeilenargumente überschreiben Konfigurationsdatei", "[config]") {
    std::string output = run_cmd("./img_to_ascii --load-config ../example_config.json --width 150 --fps 20");
    REQUIRE(output.find("Konfigurationsdatei geladen") != std::string::npos);
    REQUIRE(output.find("Breite: 150") != std::string::npos);
    REQUIRE(output.find("Frame-Rate: 20") != std::string::npos);
}