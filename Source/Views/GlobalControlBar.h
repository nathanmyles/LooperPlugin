#pragma once

#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

/**
 * GlobalControlBar - Top-level controls for the multi-track looper
 *
 * Contains:
 * - Play button (global play/stop)
 * - Monitor button (input monitoring)
 * - Clear All button
 * - Undo Last button (on last modified track or global undo)
 * - Status/Info display
 */
class GlobalControlBar : public juce::Component,
                         public juce::AudioProcessorValueTreeState::Listener
{
public:
    // Callbacks
    std::function<void(bool)> onPlayChanged;
    std::function<void(bool)> onMonitorChanged;
    std::function<void()> onClearAll;
    std::function<void()> onUndoLast;

    GlobalControlBar(juce::AudioProcessorValueTreeState& parameters);
    ~GlobalControlBar() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // AudioProcessorValueTreeState::Listener
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // Update status display
    void setStatusText(const juce::String& text);
    void setLoopInfo(int trackCount, int totalLoops);

private:
    juce::AudioProcessorValueTreeState& parameters;

    // Controls
    juce::TextButton playButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> playAttachment;

    juce::TextButton monitorButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> monitorAttachment;

    juce::TextButton clearAllButton;
    juce::TextButton undoLastButton;

    // Labels
    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::Label infoLabel;

    void setupComponents();
    void updateButtonStyles();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalControlBar)
};
