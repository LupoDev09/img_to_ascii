//
// Created by lupo on 04.07.26.
//

#ifndef IMG_TO_ASCII_GENERATEFRAMES_H
#define IMG_TO_ASCII_GENERATEFRAMES_H
#include <filesystem>

class GenerateFrames {
    public:
    /**
     * @brief Generate frames from a video file using ffmpeg
     * @details If you want to change this function you have to make sure that the output frames are under frames/ and in the format frame_%06d.png
     * @param input_path the path to the image
     * @param frame_rate the Frame rate (Used to generate the appropriate number of frames)
     * @param width the width of the output ascii video
     * @param height the height of the output ascii video
     */
    static void generate(const std::filesystem::path &input_path, int frame_rate, int width, int height);
};

#endif// IMG_TO_ASCII_GENERATEFRAMES_H
