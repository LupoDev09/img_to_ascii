//
// Created by lupo on 04.07.26.
//

#ifndef IMG_TO_ASCII_ASCIIMAPPER_H
#define IMG_TO_ASCII_ASCIIMAPPER_H
#include "Data_structures/Frame.h"

#include <string>



class AsciiMapper {
    std::string charset = " .:-=+*#%@";

public:
    /**
     * @brief Changes the charset used for mapping pixel luminance to ASCII characters.
     * @param New_charset the new charset to use
     */
    void changeCharset(const std::string& New_charset) {
        this->charset = New_charset;
    }

    /**
     * @brief Maps a Frame to an AsciiFrame using the current charset.
     * @param frame the frame to map to ASCII characters
     * @return the Generated Ascii Frame
     */
    AsciiFrame map(const Frame& frame) {
        // TODO: Implement
        return AsciiFrame{};
    }
};



#endif// IMG_TO_ASCII_ASCIIMAPPER_H
