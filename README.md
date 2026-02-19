# LooperPlugin

A multi-loop audio looper plugin built with the JUCE framework. Record multiple synchronized loops with automatic timing and layer them for rich, complex sounds.

## Features

- **Multi-Loop Recording**: Record up to 8 loops that automatically synchronize to the first loop
- **Automatic Loop Length**: The first loop sets the base length; all subsequent loops are constrained to match
- **Loop Synchronization**: Overdubs can be recorded at any position within the loop and maintain their timing
- **Input Monitoring**: Toggle to control whether input audio passes through to output (prevents feedback when using microphones)
- **Undo**: Remove the last recorded loop individually
- **Clear All**: Reset and start fresh
- **Volume Control**: Adjust playback volume

## Requirements

- JUCE 8.0 or later
- CMake 3.15 or later
- C++17 compatible compiler

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

### Testing

The project uses Google Test (GTest) for unit testing.

```bash
cd build
ctest
```

Or run the test executable directly:

```bash
./build/LooperPluginTests
```

Tests are located in the `Tests/` directory.

## Usage

1. Load the LooperPlugin in your DAW
2. Click **Record** to start the base loop
3. Click **Stop** (Record button changes to Stop) when done - this sets the loop length
4. Click **Play** to start looping
5. Click **Record** again to overdub additional loops - they will sync to the base loop
6. Click **Undo** to remove the last recorded loop
7. Use **Clear All** to remove all loops and start over
8. Toggle **Monitor** to control input passthrough (OFF prevents feedback with microphones)
9. Adjust the **Volume** slider to control playback level

### Controls

- **Monitor Button**: Toggles input monitoring. Blue when ON (input passes through), grey when OFF (input muted).
- **Record Button**: Starts/stops recording. Red when recording, changes to "Stop".
- **Play Button**: Starts/stops playback. Green when playing.
- **Undo All Button**: Removes all recorded loops.
- **Undo Button**: Removes the last recorded loop.
- **Volume Slider**: Adjusts playback volume (0-100%).
- **Loop Count**: Displays the number of recorded loops.

### Loop Behavior

- **First Loop**: Sets the base length for all subsequent loops
- **Overdubs**: Can be recorded at any point in the loop cycle and maintain their position
- **Synchronization**: All loops wrap at the same time, keeping everything in time
- **Maximum Length**: Up to 60 seconds per loop

## Plugin Parameters

All parameters are automatable and can be controlled by your DAW:

| Parameter | Range | Description |
|-----------|-------|-------------|
| Volume | 0.0-1.0 | Playback volume level |
| Record | Boolean | Toggle recording on/off |
| Play | Boolean | Toggle playback on/off |
| Monitor | Boolean | Toggle input monitoring on/off |

## Technical Details

- **Sample Rate**: Supports common sample rates (44.1kHz, 48kHz, 96kHz)
- **Buffer Size**: Optimized for real-time performance
- **Channels**: Stereo input/output
- **Latency**: Low latency design suitable for live performance
- **Memory**: Circular buffers for efficient multi-loop storage
- **Crossfade**: Automatic crossfading at loop boundaries to prevent clicks

## Development

The plugin is structured as follows:

```
Source/
├── PluginProcessor.h/cpp   # Audio processing engine (DAW interface)
├── PluginEditor.h/cpp      # Main editor component
├── Looper.h/cpp            # Core looping logic and audio processing
└── LooperView.h/cpp        # UI controls for the looper

Tests/
├── test_main.cpp           # Test runner
└── test_looper.cpp         # Looper unit tests
```

### Architecture

- `LooperAudioProcessor`: DAW interface, manages plugin lifecycle and parameters
- `LooperAudioProcessorEditor`: Main plugin editor, hosts the LooperView
- `Looper`: Core looping engine, manages multiple synchronized loops, recording, and playback
- `LooperView`: UI component with all looper controls (buttons, sliders, displays)
- Thread-safe communication between UI and audio threads via atomic flags

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
- If experiencing feedback, turn OFF the Monitor button

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
