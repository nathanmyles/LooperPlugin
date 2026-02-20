# LooperPlugin

A multi-track audio looper plugin built with the JUCE framework. Record multiple synchronized loops across unlimited tracks, with each track supporting multiple overdubbed layers.

## Features

- **Multi-Track Support**: Unlimited tracks that can be added and removed dynamically
- **Multi-Loop Recording**: Each track supports multiple synchronized overdubs
- **Automatic Loop Length**: The first loop sets the base length; all subsequent loops on all tracks sync to it
- **Per-Track Controls**: Volume, Mute, Solo, and Record for each track
- **Global Controls**: Play/Stop, Input Monitoring, Clear All, and Undo Last
- **Solo Logic**: When any track is soloed, only soloed tracks play (standard DAW behavior)
- **Input Monitoring**: Toggle to control whether input audio passes through to output (prevents feedback when using microphones)
- **Undo**: Remove the last recorded loop globally or per-track
- **Clear All**: Reset all tracks or clear individual tracks
- **Volume Control**: Per-track volume sliders plus effective volume based on mute/solo state

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

The build system automatically installs plugins to the system directories:
- **macOS AU**: `~/Library/Audio/Plug-Ins/Components/`
- **macOS VST3**: `~/Library/Audio/Plug-Ins/VST3/`

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
2. The plugin starts with one track. Click **+ Add Track** to add more tracks as needed
3. Click the **REC** button on any track to start recording (only one track can record at a time)
4. Click **REC** again to stop - this sets the loop length for all tracks
5. Click **Play** in the top bar to start looping all tracks
6. Record on other tracks to layer sounds - all tracks sync to the same loop length
7. Use **Mute** (M) and **Solo** (S) buttons per-track to control which tracks play
8. Adjust per-track **Volume** sliders to mix levels
9. Use **Clear** on a track to remove all its loops, or **Undo** to remove the last loop
10. Use **Clear All** in the top bar to clear all tracks at once
11. Toggle **Monitor** to control input passthrough (OFF prevents feedback with microphones)

### Global Controls (Top Bar)

- **Play Button**: Starts/stops playback for all tracks. Green when playing.
- **Monitor Button**: Toggles input monitoring. Blue when ON (input passes through).
- **Clear All Button**: Removes all loops from all tracks.
- **Undo Last Button**: Removes the most recently recorded loop across all tracks.

### Per-Track Controls

Each track has the following controls (from top to bottom):

- **Track Name**: Displayed at top (e.g., "Track 1")
- **Volume Slider**: Vertical slider (0-100%) controlling track volume
- **REC Button**: Toggle recording on/off. Red when recording.
- **M Button**: Mute toggle. Orange when muted.
- **S Button**: Solo toggle. Yellow when soloed.
- **Clear Button**: Removes all loops from this track.
- **Undo Button**: Removes the last loop from this track.
- **X Button**: Removes this track entirely.
- **Loop Count**: Shows number of recorded loops on this track.

### Track Behavior

- **Only One Track Records at a Time**: Starting recording on one track automatically stops recording on any other track
- **Shared Loop Length**: All tracks share the same loop length, set by the first recorded loop
- **Mute**: Silences a track entirely
- **Solo**: When any track is soloed, only soloed (unmuted) tracks play
- **Volume**: Effective volume considers both the slider and mute/solo state
- **Maximum Length**: Up to 60 seconds per loop

## Plugin Parameters

All parameters are automatable and can be controlled by your DAW:

| Parameter |  Range   | Description                          |
|-----------|----------|--------------------------------------|
| Play      | Boolean  | Toggle playback on/off (all tracks)  |
| Monitor   | Boolean  | Toggle input monitoring on/off       |

Note: Per-track controls (volume, mute, solo, record) are managed internally and not exposed as DAW-automatable parameters.

## Technical Details

- **Sample Rate**: Supports common sample rates (44.1kHz, 48kHz, 96kHz)
- **Buffer Size**: Optimized for real-time performance
- **Channels**: Stereo input/output
- **Latency**: Low latency design suitable for live performance
- **Memory**: Circular buffers for efficient multi-loop storage per track
- **Crossfade**: Automatic crossfading at loop boundaries to prevent clicks
- **Thread-Safe**: UI and audio thread communication via atomic flags

## Development

The plugin is structured as follows:

```
Source/
├── PluginProcessor.h/cpp      # Audio processing engine (DAW interface)
├── PluginEditor.h/cpp         # Main editor component
├── Models/                    # Data models and business logic
│   ├── Looper.h/cpp           # Core looping logic (per-track)
│   ├── Track.h/cpp            # Track management (volume, mute, solo)
│   └── TrackManager.h/cpp     # Shared timing across all tracks
└── Views/                     # UI components
    ├── TrackView.h/cpp        # UI for a single track
    ├── TrackContainer.h/cpp   # Horizontal scrolling container for tracks
    └── GlobalControlBar.h/cpp # Top-level play/monitor controls

Tests/
├── test_main.cpp              # Test runner
└── test_looper.cpp            # Looper unit tests
```

### Architecture

- `LooperAudioProcessor`: DAW interface, manages plugin lifecycle and global parameters
- `LooperAudioProcessorEditor`: Main plugin editor, hosts TrackContainer and GlobalControlBar
- `TrackManager`: Centralized timing management shared across all tracks
- `Track`: Per-track audio processing with volume, mute, solo controls
- `Looper`: Core looping engine per track, manages multiple synchronized loops
- `TrackContainer`: Horizontal scrolling container managing all track views
- `TrackView`: UI component for a single track (buttons, sliders, displays)
- `GlobalControlBar`: Top-level controls for global play/monitor
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
