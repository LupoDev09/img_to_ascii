//
// Created by lupo on 04.07.26.
//

#ifndef IMG_TO_ASCII_DATASTRUCTURES_H
#define IMG_TO_ASCII_DATASTRUCTURES_H
#include <cstdint>
#include <string>
#include <vector>

namespace DataStructures {
    struct Pixel {
        uint8_t r, g, b;

        [[nodiscard]] float luminance() const {
            // Wahrnehmungsgewichtung (nicht einfach Mittelwert!)
            return 0.2126f * static_cast<float>(r)
                 + 0.7152f * static_cast<float>(g)
                 + 0.0722f * static_cast<float>(b);
        }
    };

    struct Frame {
        int width;
        int height;
        double source_fps;
        std::vector<Pixel> data;
    };
}

// einfache UTF-8 → UTF-32 Konvertierung (minimalistisch)
inline std::u32string utf8_to_utf32(const std::string& input) {
    std::u32string result;
    result.reserve(input.size());
    const unsigned char* data = reinterpret_cast<const unsigned char*>(input.data());
    const size_t size = input.size();

    for (size_t i = 0; i < size;) {
        unsigned char c = data[i];

        if (c < 0x80) {
            result.push_back(c);
            i++;
        } else if ((c & 0xE0) == 0xC0) {
            if (i + 1 >= size || (data[i + 1] & 0xC0) != 0x80) {
                result.push_back(U'\uFFFD');
                i++;
                continue;
            }
            char32_t cp = ((c & 0x1F) << 6) | (data[i + 1] & 0x3F);
            if (cp < 0x80) {
                result.push_back(U'\uFFFD');
            } else {
                result.push_back(cp);
            }
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            if (i + 2 >= size || (data[i + 1] & 0xC0) != 0x80 || (data[i + 2] & 0xC0) != 0x80) {
                result.push_back(U'\uFFFD');
                i++;
                continue;
            }
            char32_t cp = ((c & 0x0F) << 12) | ((data[i + 1] & 0x3F) << 6) | (data[i + 2] & 0x3F);
            if (cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF)) {
                result.push_back(U'\uFFFD');
            } else {
                result.push_back(cp);
            }
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            if (i + 3 >= size || (data[i + 1] & 0xC0) != 0x80 || (data[i + 2] & 0xC0) != 0x80 || (data[i + 3] & 0xC0) != 0x80) {
                result.push_back(U'\uFFFD');
                i++;
                continue;
            }
            char32_t cp = ((c & 0x07) << 18) | ((data[i + 1] & 0x3F) << 12) | ((data[i + 2] & 0x3F) << 6) | (data[i + 3] & 0x3F);
            if (cp < 0x10000 || cp > 0x10FFFF) {
                result.push_back(U'\uFFFD');
            } else {
                result.push_back(cp);
            }
            i += 4;
        } else {
            result.push_back(U'\uFFFD');
            i++;
        }
    }

    return result;
}

inline std::u32string to_u32(const std::string& s) {
    std::u32string result;
    result.reserve(s.size());
    for (unsigned char c : s) {
        result.push_back(static_cast<char32_t>(c));
    }
    return result;
}

inline std::string utf32_to_utf8(const char32_t cp) {
    std::string out;

    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>((cp >> 6) | 0xC0);
        out += static_cast<char>((cp & 0x3F) | 0x80);
    } else if (cp < 0x10000) {
        out += static_cast<char>((cp >> 12) | 0xE0);
        out += static_cast<char>(((cp >> 6) & 0x3F) | 0x80);
        out += static_cast<char>((cp & 0x3F) | 0x80);
    } else if (cp <= 0x10FFFF) {
        out += static_cast<char>((cp >> 18) | 0xF0);
        out += static_cast<char>(((cp >> 12) & 0x3F) | 0x80);
        out += static_cast<char>(((cp >> 6) & 0x3F) | 0x80);
        out += static_cast<char>((cp & 0x3F) | 0x80);
    } else {
        out += static_cast<char>(0xEF);
        out += static_cast<char>(0xBF);
        out += static_cast<char>(0xBD);
    }

    return out;
}

#endif// IMG_TO_ASCII_DATASTRUCTURES_H
