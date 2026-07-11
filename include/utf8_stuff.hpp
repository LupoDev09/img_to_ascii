//
// Created by lupo on 11.07.26.
//

#ifndef IMG_TO_ASCII_UTF8_STUFF_HPP
#define IMG_TO_ASCII_UTF8_STUFF_HPP
#include <string>

namespace utf_8_stuff {
    /**
     * @brief Convert UTF-8 encoded string to UTF-32 character sequence.
     * @param input UTF-8 encoded string
     * @return UTF-32 string; invalid sequences are replaced with replacement character (U+FFFD)
     *
     * Handles all valid UTF-8 sequences (1-4 byte sequences).
     * Invalid byte sequences are converted to the Unicode replacement character.
     */
    std::u32string utf8_to_utf32(const std::string &input);

    /**
     * @brief Simple conversion from ASCII string to UTF-32.
     * @param s ASCII/single-byte encoded string
     * @return UTF-32 string where each character is zero-extended to 32 bits
     *
     * Only works correctly with ASCII strings (0-127 range).
     * For UTF-8 input, use utf8_to_utf32() instead.
     */
    std::u32string ascii_to_u32(const std::string &s);

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
    std::string utf32_to_utf8(char32_t cp);
}

#endif// IMG_TO_ASCII_UTF8_STUFF_HPP
