//
// Created by lupo on 04.07.26.
//

#ifndef IMG_TO_ASCII_FRAMESOURCE_H
#define IMG_TO_ASCII_FRAMESOURCE_H
#include <Data_structures/Frame.h>

class FrameSource {
public:
    virtual ~FrameSource() = default;

    // Gibt false zurück, wenn keine Frames mehr kommen
    virtual bool next(Frame& frame) = 0;
};

class ImageSequenceSource : public FrameSource {
    // lädt frame_0001.png etc.
    // TODO: Diese Klasse soll die next func implementieren und den nächsten frame als Bild laden
};

#endif// IMG_TO_ASCII_FRAMESOURCE_H
