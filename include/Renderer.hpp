//
// Created by lupo on 04.07.26.
//

#ifndef IMG_TO_ASCII_RENDERER_H
#define IMG_TO_ASCII_RENDERER_H
#include <OutputWriter.hpp>
#include <SyncClock.hpp>
#include <array>
#include <dataStructures.hpp>
#include <filesystem>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

class AudioPlayer;
/**
 * @class Renderer
 * @brief Converts video frames into ASCII art with optional color support.
 *
 * The Renderer maps pixel luminance values to characters from a configurable charset,
 * creating terminal-displayable ASCII art. It can output with ANSI color codes for
 * colored terminals or plain text for standard terminals.
 */
class Renderer {
public:
    Renderer() = delete;
    Renderer(bool no_audio, bool no_output, AudioPlayer* audio, int frame_rate, OutputWriter* output_writer);
    ~Renderer();

    /**
     * @brief Starts the rendering process in a separate thread.
     */
    void start_rendering();

    /**
     * @brief Adds a decoded video frame to the rendering queue.
     * @param frame the frame to add
     * @return whether the adding of the frame was successful
     */
    bool add_decoded_frame(const DataStructures::Frame& frame);

    /**
     * @brief Signals that no new frames will be added to the queue.
     */
    void stop();

    // Keine Kopien oder Zuweisungen
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;


    /**
     * @struct Config
     * @brief Rendering configuration options.
     */
    struct Config {
        /// Enable ANSI color codes in output (true = colored, false = grayscale)
        bool color = true;

        /// Character palette for luminance mapping (darker to lighter characters)
        std::u32string charset = U" ░▒▓█";

        /// Left padding for each line of ASCII art
        int left_pad = 0;
    } config;

    /**
     * @brief Sets the character set for luminance mapping.
     * @param charset the character set to use
     */
    void set_charset(const std::u32string& charset);

    /**
     * @brief Sets the left padding for each line of ASCII art.
     * @param left_pad the left padding to use
     */
    void set_left_pad(int left_pad);


    /// Callback type for frame processing. Frame ownership is transferred to the callback.
    using FrameCallback = std::function<void(DataStructures::Frame&& Frame)>;

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
    static void decode_frames(const std::filesystem::path& input_path, int frame_rate, int width, int height,
            const FrameCallback& on_frame);

private:
    std::array<std::string, 256> m_number_lut;///< Lookup table for luminance to character mapping
    std::array<std::string, 256> m_char_lut;  ///< Lookup table for character mapping

    std::string m_left_pad_str;

    std::mutex m_queue_mutex;
    std::queue<DataStructures::Frame> m_frame_queue;
    std::thread worker_thread_;

    bool no_audio_;
    bool no_output_;
    int frame_rate_;

    OutputWriter* output_writer_;
    SyncClock clock_;
    AudioPlayer* audio_;
    std::atomic<bool> m_no_new_frames_;

    static constexpr std::uint8_t QUEUE_MAX_SIZE = 20;
    static constexpr std::string_view COLOR_PREFIX = "\033[38;2;";
    static constexpr std::string_view COLOR_RESET = "\033[0m";

    /**
     * @brief Worker thread function that processes frames from the queue.
     */
    void build_char_lut();

    /**
     * @brief Builds the left padding string based on the configured left_pad value.
     */
    void build_padding();


    /**
     * @brief Render a video frame into ASCII art.
     *
     * Converts pixel data to ASCII characters based on luminance values.
     * Output includes ANSI terminal codes for positioning and optionally coloring.
     *
     * @param frame The decoded video frame to render
     * @return String containing ANSI-formatted ASCII art with embedded control codes
     */
    [[nodiscard]] std::string render_frame(const DataStructures::Frame& frame) const;
};

#endif// IMG_TO_ASCII_RENDERER_H
