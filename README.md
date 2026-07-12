# img_to_ascii

A CLI tool that converts video/image files into real-time ASCII art animations displayed in the terminal, with synchronized audio playback and optional color support.

## TODOs
- [x] Find a way to save the audio in memory instead of saving it in a temporary file
- [ ] Add some kind of progress bar for the audio playback
- [ ] Add a way to skip frames if the processing is too slow
- [ ] Add a way to render the subtitle stream of the video if it exists
- [ ] Maybe render and decode the video on the GPU if available
- [ ] Add more TODOs for the project

## Features

- **Video-to-ASCII Conversion**: Converts any video format supported by FFmpeg into ASCII art
- **Real-time Playback**: Displays ASCII art at the specified frame rate with frame-perfect timing
- **Audio Synchronization**: Extracts and plays audio from video files synchronized with the ASCII animation
- **Flexible Sizing**: Control output dimensions independently or auto-scale to preserve aspect ratio
- **Color Support**: Optional ANSI color codes for colored terminal output
- **Custom Character Palettes**: Customize the character set used for luminance mapping
- **Graceful Degradation**: Handles videos without audio or with various codecs seamlessly

## Building

### Dependencies

- **CMake** 3.10+
- **FFmpeg development libraries** (libavformat, libavcodec, libswresample)
- **miniaudio** (included in the project)
- **C++17 or later** compiler

### Build Instructions

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Usage

### Basic Usage

Convert a video to ASCII art at default size and frame rate:

```bash
./img_to_ascii --input video.mp4
```

### Command-line Options

#### Input
- `-i, --input <file>` **[Required]** Path to the video file to convert

#### Sizing
- `-w, --width <pixels>` Width of the output ASCII video (default: 0 = auto-calculate)
- `-h, --height <pixels>` Height of the output ASCII video (default: 0 = auto-calculate)

At least one of width or height must be specified as a positive integer.

#### Playback
- `-f, --fps <rate>` Frame rate for output (default: 0 = use source video's FPS)
- `--no-audio` Disable audio playback during animation

#### Output
- `--no-color` Disable ANSI color output (grayscale only)
- `-c, --charset <chars>` Character palette for ASCII mapping (default: " ░▒▓█")
  - Characters should be ordered from darkest to brightest
  - Default uses Unicode block elements for smooth gradation

#### General
- `-v, --verbose` Enable verbose diagnostic output to stderr
- `--help` Display help message
- `--no-output` Process video without outputting ASCII animation to stdout

### Examples

Convert with custom dimensions:
```bash
./img_to_ascii --input video.mp4 --width 120 --height 40
```

Convert with custom frame rate and character set:
```bash
./img_to_ascii --input video.mp4 --fps 24 --charset "@%#*+=-:. "
```

Convert to grayscale without audio:
```bash
./img_to_ascii --input video.mp4 --no-color --no-audio --width 100
```

Process without output (useful for performance testing):
```bash
./img_to_ascii --input video.mp4 --no-output --verbose
```

## Project Structure

```
include/
  ├── dataStructures.h      # Core data structures (Pixel, Frame) and UTF-8/UTF-32 conversion utilities
  ├── Renderer.h             # ASCII art rendering engine
  ├── GenerateFrames.h       # Video decoding and frame extraction
  ├── AudioPlayer.h          # Audio extraction and playback
  ├── SyncClock.h            # High-precision timing for frame synchronization
  ├── cxxopts.hpp            # Command-line argument parsing library
  └── miniaudio.h            # Audio playback library (header-only)
src/
  ├── main.cpp               # Application entry point and CLI logic
  ├── Renderer.cpp           # Renderer implementation
  ├── GenerateFrames.cpp     # Frame generation implementation
  └── AudioPlayer.cpp        # (if separate implementation exists)
```

## Architecture

### Data Flow

1. **Video Decoding** (GenerateFrames)
   - Uses FFmpeg to decode video frames
   - Scales frames to target dimensions
   - Extracts frame timing information

2. **Audio Extraction** (AudioPlayer)
   - Decodes audio stream from video
   - Converts to standardized format (Stereo, 44.1kHz, S16 PCM)
   - Writes to temporary WAV file for playback

3. **Rendering** (Renderer)
   - Maps pixel luminance to character based on charset
   - Optionally applies ANSI color codes
   - Generates ANSI terminal control codes for positioning

4. **Synchronization** (SyncClock)
   - Maintains precise frame timing
   - Synchronizes ASCII output with audio playback
   - Prevents timing drift across long videos

5. **Output**
   - Writes ASCII art to stdout with ANSI codes
   - Clears and repositions display for each frame

### Key Classes

#### Renderer
Converts video frames into ASCII art with optional color support. Maps pixel luminance values to characters from a configurable charset.

#### GenerateFrames
Decodes video files using FFmpeg and provides frames via callback mechanism. Handles scaling, frame rate resampling, and metadata extraction.

#### AudioPlayer
Manages audio extraction from video files and synchronized playback. Handles files without audio gracefully.

#### SyncClock
Provides high-precision timing for frame-perfect playback. Uses absolute target times to prevent drift over long videos.

## Character Mapping

The renderer converts pixel luminance to ASCII characters using a linear mapping:
- Darkest pixels → First character in charset (typically space or .)
- Brightest pixels → Last character in charset (typically @ or █)

Luminance is calculated using standard perception weights (BT.709):
```
L = 0.2126*R + 0.7152*G + 0.0722*B
```

This better matches human visual perception than simple averaging.

## Performance Considerations

- **Memory**: Frames are processed one at a time; memory usage is proportional to output dimensions
- **CPU**: Real-time playback requires reasonable processing power; reduce dimensions for lower-end systems
- **I/O**: Audio is extracted to temporary WAV file; ensure sufficient disk space
- **Timing**: Uses high-resolution timer (steady_clock) for frame-perfect timing

## Troubleshooting

### Video not displaying
- Ensure FFmpeg is installed: `ffmpeg -version`
- Check video format is supported
- Try with `--verbose` flag for diagnostic output

### Audio not playing
- Verify video contains audio: `ffmpeg -i video.mp4`
- Check miniaudio can access audio device
- Try `--no-audio` flag to skip audio playback

### Timing issues
- Reduce output dimensions to improve frame processing speed
- Ensure terminal is not redirected or buffering output
- Check system load and available CPU

### Character display issues
- Ensure terminal supports UTF-8 encoding
- For grayscale output, use `--no-color`
- Try different character sets with `--charset`

## License

Use this stuff like you want. \
No warranty. \
I won't fix your problems, neither with this software nor your personal ones.

## Contributing

Please don't this is just, I don't know how to say it in English in Germany we say "Beschäftigungs-Therapie".
