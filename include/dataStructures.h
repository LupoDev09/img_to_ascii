//
// Created by lupo on 04.07.26.
//

#ifndef IMG_TO_ASCII_DATASTRUCTURES_H
#define IMG_TO_ASCII_DATASTRUCTURES_H
#include <cstdint>
#include <string>
#include <vector>

namespace DataStructures {
    /**
     * @struct Pixel
     * @brief Represents an RGB pixel with 8 bits per channel.
     */
    struct Pixel {
        uint8_t r; ///< Red channel (0-255)
        uint8_t g; ///< Green channel (0-255)
        uint8_t b; ///< Blue channel (0-255)

        /**
         * @brief Calculate the perceived luminance of the pixel.
         * @return Luminance value using standard perception weights (BT.709).
         * 
         * Uses the formula: L = 0.2126*R + 0.7152*G + 0.0722*B
         * This weighted average better matches human perception than simple averaging.
         */
        [[nodiscard]] float luminance() const {
            return 0.2126f * static_cast<float>(r)
                 + 0.7152f * static_cast<float>(g)
                 + 0.0722f * static_cast<float>(b);
        }
    };

    /**
     * @struct Frame
     * @brief Represents a decoded video frame with RGB pixel data.
     */
    struct Frame {
        int width;               ///< Frame width in pixels
        int height;              ///< Frame height in pixels
        double source_fps;       ///< Original frame rate from the source video
        std::vector<Pixel> data; ///< Pixel data in row-major order (width * height pixels)
    };
}

/**
 * @brief Convert UTF-8 encoded string to UTF-32 character sequence.
 * @param input UTF-8 encoded string
 * @return UTF-32 string; invalid sequences are replaced with replacement character (U+FFFD)
 * 
 * Handles all valid UTF-8 sequences (1-4 byte sequences).
 * Invalid byte sequences are converted to the Unicode replacement character.
 */
inline std::u32string utf8_to_utf32(const std::string& input) {
    std::u32string result;
    result.reserve(input.size());
    const auto* data = reinterpret_cast<const unsigned char*>(input.data());
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

/**
 * @brief Simple conversion from ASCII string to UTF-32.
 * @param s ASCII/single-byte encoded string
 * @return UTF-32 string where each character is zero-extended to 32 bits
 * 
 * Only works correctly with ASCII strings (0-127 range).
 * For UTF-8 input, use utf8_to_utf32() instead.
 */
inline std::u32string to_u32(const std::string& s) {
    std::u32string result;
    result.reserve(s.size());
    for (unsigned char c : s) {
        result.push_back(static_cast<char32_t>(c));
    }
    return result;
}

/**
 * @brief Convert a single UTF-32 code point to UTF-8 byte sequence.
 * @param cp Unicode code point (0x0 to 0x10FFFF)
 * @return UTF-8 encoded string; invalid code points are converted to U+FFFD (replacement character)
 * 
 * Supports all valid Unicode ranges:
 * - 1-byte: 0x00 - 0x7F
 * - 2-byte: 0x80 - 0x7FF
 * - 3-byte: 0x800 - 0xFFFF
 * - 4-byte: 0x10000 - 0x10FFFF
 * 
 * Code points outside valid ranges are replaced with the Unicode replacement character (U+FFFD).
 */
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
