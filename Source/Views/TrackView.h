#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Models/Track.h"

/**
 * TrackView - UI component for a single track
 *
 * Displays controls for:
 * - Track name/label
 * - Volume slider
 * - Record button
 * - Mute button
 * - Solo button
 * - Remove button
 * - Loop count display
 */
class TrackView : public juce::Component
{
public:
    // Callbacks
    std::function<void(int)> onRemoveTrack;
    std::function<void(int, bool)> onRecordClicked;
    std::function<void(int)> onClearTrack;
    std::function<void(int)> onUndoTrack;

    TrackView(int trackId, Track& track);
    ~TrackView() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Update UI to reflect track state
    void updateFromTrack();

    // Refresh the loop count display
    void refreshLoopCount();

    int getTrackId() const { return trackId; }

private:
    int trackId;
    Track& track;

    // UI Components
    juce::Label trackNameLabel;
    juce::Slider volumeSlider;
    juce::TextButton recordButton;
    juce::TextButton muteButton;
    juce::TextButton soloButton;
    juce::TextButton clearButton;
    juce::TextButton undoButton;
    juce::TextButton removeButton;
    juce::Label loopCountLabel;
    juce::Label statusLabel;

    // State tracking
    bool wasRecording = false;
    int lastLoopCount = 0;

    void setupComponents();
    void updateButtonStyles();
    void updateStatusLabel();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackView)
};
