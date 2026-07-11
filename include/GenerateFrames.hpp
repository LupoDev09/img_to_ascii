//
// Created by lupo on 04.07.26.
//

#ifndef IMG_TO_ASCII_GENERATEFRAMES_H
#define IMG_TO_ASCII_GENERATEFRAMES_H
#include <filesystem>

#include <dataStructures.hpp>
#include <functional>

/**
 * @class GenerateFrames
 * @brief Decodes video files and provides frames for processing.
 * 
 * Handles video file decoding and frame extraction with optional scaling.
 * Frames are provided via a callback mechanism for memory efficiency.
 */
class GenerateFrames {
    public:
    /// Callback type for frame processing. Frame ownership is transferred to the callback.
    using FrameCallback = std::function<void(DataStructures::Frame && Frame)>;

    /**
     * @brief Decode a video file and deliver scaled RGB frames via callback.
     * 
     * Extracts frames from a video file, optionally scales them to the specified dimensions,
     * and invokes the callback for each decoded frame. The caller receives frame data
     * directly suitable for rendering.
     * 
     * @param input_path Path to the video file to decode
     * @param frame_rate Target frame rate (0 = use source file's fps, >0 = resample to this fps)
     * @param width Target width in pixels (0 = preserve aspect ratio, >0 = scale to this width)
     * @param height Target height in pixels (0 = preserve aspect ratio, >0 = scale to this height)
     * @param on_frame Callback invoked for each decoded frame with ownership transfer
     * 
     * @note At least one of width or height must be specified (non-zero)
     * @note Frames are delivered in playback order
     * @note If only one dimension is specified, the other is calculated to preserve aspect ratio
     */
    static void generate(const std::filesystem::path& input_path, int frame_rate, int width, int height, const FrameCallback &on_frame);
};

#endif// IMG_TO_ASCII_GENERATEFRAMES_H
