# LooperPlugin

A simple audio looping plugin built with the JUCE framework. This plugin allows you to record audio input and play it back in a loop with adjustable length.

## Features

- **Record**: Capture up to 10 seconds of audio input
- **Play**: Loop recorded audio with adjustable length (0.1-10 seconds)
- **Clear**: Reset the loop buffer
- **Real-time controls**: Start/stop recording and playback on the fly
- **Adjustable loop length**: Control playback duration independently from recording length

## Requirements

- JUCE 8.0 or later
- CMake 3.15 or later
- C++17 compatible compiler
- macOS (with Xcode), Windows (with Visual Studio), or Linux

## Building

### 1. Build with CMake

```bash
mkdir build
cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
cmake --build .
```

This will build the plugin in the following formats:
- VST3
- AU (Audio Units) - macOS only
- Standalone application

### 2. Installation

  - **macOS**: Build artifacts will be placed in `build/LooperPlugin_artefacts/VST3` and `build/LooperPlugin_artefacts/AU`
  - **Windows**: VST3 files will be in `build/LooperPlugin_artefacts/VST3`
  - **Linux**: VST3 files will be in `build/LooperPlugin_artefacts/VST3`

Install the plugin files to your DAW's plugin directory.

## Usage

1. Load the LooperPlugin in your DAW
2. Click **Record** to capture audio input (up to 10 seconds)
3. Click **Stop** when you're done recording
4. Adjust the **Loop Length** slider to set playback duration
5. Click **Play** to loop the recorded audio
6. Use **Clear** to reset and record a new loop

### Controls

- **Record Button**: Starts/stops recording. Red when recording.
- **Play Button**: Starts/stops playback. Green when playing.
- **Clear Button**: Clears the recorded audio buffer.
- **Loop Length Slider**: Adjusts the playback loop duration (0.1-10 seconds).
- **Status Display**: Shows current state (Ready, Recording..., Playing..., Cleared).

## Plugin Parameters

All parameters are automatable and can be controlled by your DAW:

| Parameter | Range | Description |
|-----------|-------|-------------|
| Loop Length | 0.1-10.0 seconds | Duration of the playback loop |
| Record | Boolean | Toggle recording on/off |
| Play | Boolean | Toggle playback on/off |
| Clear | Boolean | Clears the loop buffer (momentary) |

## Technical Details

- **Sample Rate**: Supports common sample rates (44.1kHz, 48kHz, 96kHz)
- **Buffer Size**: Optimized for real-time performance
- **Channels**: Stereo input/output
- **Latency**: Low latency design suitable for live performance
- **Memory**: Uses circular buffer for efficient audio storage

## Development

The plugin is structured as follows:

```
Source/
├── PluginProcessor.h/cpp  # Audio processing engine
└── PluginEditor.h/cpp      # User interface
```

### Architecture

- `LooperAudioProcessor`: Core audio processing, manages recording/playback state
- `LooperAudioProcessorEditor`: GUI controls and parameter display
- Circular buffer implementation for efficient audio looping
- Parameter automation via JUCE's AudioProcessorParameter system

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## Troubleshooting

### Plugin not loading in DAW
- Ensure JUCE is properly built as a submodule
- Check that your DAW supports the plugin format (VST3/AU)
- Verify the plugin is installed in the correct directory

### Audio issues
- Check input/output routing in your DAW
- Ensure proper sample rate settings
- Verify buffer size settings are compatible

### Build errors
- Make sure all submodules are initialized
- Check CMake version compatibility
- Verify compiler requirements (C++17)
- JUCE is automatically fetched via CPM during the CMake configure step

## Support

For issues and questions:
- Open an issue on the GitHub repository
- Check the JUCE documentation for framework-specific questions
- Review existing issues for common solutions