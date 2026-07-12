//
// Created by lupo on 11.07.26.
//

#include <utf8_stuff.hpp>

std::u32string utf_8_stuff::utf8_to_utf32(const std::string &input) {
    std::u32string result;// Holds the resulting utf-8 string
    result.reserve(input.size());
    const auto *data = reinterpret_cast<const unsigned char *>(input.data());// Pointer to the string data
    const size_t size = input.size();

    for (size_t i = 0; i < size;) {     // For every Char in the string
        const unsigned char c = data[i];// Represents the current char in the string

        if (c < 0x80) {
            result.push_back(c);
            i++;
        } else if ((c & 0xE0) == 0xC0) {
            if (i + 1 >= size || (data[i + 1] & 0xC0) != 0x80) {// If the next char is not a valid continuation byte
                result.push_back(U'\uFFFD');                    // Set the char to U+FFFD
                i++;
                continue;
            }
            const char32_t cp = ((c & 0x1F) << 6) | (data[i + 1] & 0x3F);
            if (cp < 0x80) {
                result.push_back(U'\uFFFD');// the char is invalid
            } else {
                result.push_back(cp);
            }
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            if (i + 2 >= size || (data[i + 1] & 0xC0) != 0x80 || (data[i + 2] & 0xC0) != 0x80) {
                result.push_back(U'\uFFFD');// the char is invalid
                i++;
                continue;
            }
            const char32_t cp = ((c & 0x0F) << 12) | ((data[i + 1] & 0x3F) << 6) | (data[i + 2] & 0x3F);
            if (cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF)) {
                result.push_back(U'\uFFFD');// the char is invalid
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

            const char32_t cp = ((c & 0x07) << 18) | ((data[i + 1] & 0x3F) << 12) | ((data[i + 2] & 0x3F) << 6) | (data[i + 3] & 0x3F);
            if (cp < 0x10000 || cp > 0x10FFFF) {
                result.push_back(U'\uFFFD');// the char is invalid
            } else {
                result.push_back(cp);
            }
            i += 4;
        } else {
            result.push_back(U'\uFFFD');// the char is invalid
            i++;
        }
    }

    return result;
}

std::u32string utf_8_stuff::ascii_to_u32(const std::string &s) {
    std::u32string result;
    result.reserve(s.size());
    for (const unsigned char c: s) {
        result.push_back(c);
    }
    return result;
}

std::string utf_8_stuff::utf32_to_utf8(const char32_t cp) {
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
