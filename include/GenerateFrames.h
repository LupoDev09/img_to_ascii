//
// Created by lupo on 04.07.26.
//

#ifndef IMG_TO_ASCII_GENERATEFRAMES_H
#define IMG_TO_ASCII_GENERATEFRAMES_H
#include <filesystem>

#include <dataStructures.h>
#include <functional>

class GenerateFrames {
    public:
    using FrameCallback = std::function<void(DataStructures::Frame && Frame)>;

    /**
     * @brief Decode a video file and return scaled RGB frames in memory.
     * The caller can hand the result directly to the renderer.
     * @param input_path the path to the image
     * @param frame_rate the fps to sample the frames at (0 = use source fps)
     * @param width the width used as target
     * @param height the height used as target
     * @param on_frame the function to call when a frame is redy
     */
    static void generate(const std::filesystem::path& input_path, int frame_rate, int width, int height, const FrameCallback &on_frame);
};

#endif// IMG_TO_ASCII_GENERATEFRAMES_H
