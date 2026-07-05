//
// Created by lupo on 04.07.26.
//

#ifndef IMG_TO_ASCII_GENERATEFRAMES_H
#define IMG_TO_ASCII_GENERATEFRAMES_H
#include <filesystem>
#include <vector>

#include <dataStructures.h>

class GenerateFrames {
    public:
    /**
     * Decode a video file and return scaled RGB frames in memory.
     * The caller can hand the result directly to the renderer.
     *
     * @param frames the vector to save the frames in
     * @param input_path input video or GIF path
     * @param frame_rate target frame rate for sampling
     * @param width output frame width
     * @param height output frame height
     * @return decoded frames in RGB format
     */
    static void generate(std::vector<DataStructures::Frame> &frames, const std::filesystem::path &input_path, int frame_rate, int width, int height);
};

#endif// IMG_TO_ASCII_GENERATEFRAMES_H
