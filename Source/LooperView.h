#pragma once

#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Looper.h"

/**
 * LooperView - UI component for controlling the looper
 * 
 * Contains all UI controls for the looper: volume slider, record/play buttons,
 * clear/undo buttons, and status displays.
 */
class LooperView : public juce::Component,
                   public juce::AudioProcessorValueTreeState::Listener
{
public:
    // Callbacks for actions that need to be handled by the parent
    std::function<void()> onClearAll;
    std::function<void()> onUndoLast;

    LooperView(juce::AudioProcessorValueTreeState& parameters,
               const std::vector<std::unique_ptr<struct Looper::Loop>>& loops);
    ~LooperView() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Update the loop count display
    void updateLoopCount();

    // AudioProcessorValueTreeState::Listener
    void parameterChanged(const juce::String& parameterID, float newValue) override;

private:
    juce::AudioProcessorValueTreeState& parameters;
    const std::vector<std::unique_ptr<Looper::Loop>>& loops;

    // Volume controls
    juce::Slider volumeSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;
    juce::Label volumeLabel;

    // Transport buttons
    juce::TextButton recordButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> recordAttachment;

    juce::TextButton playButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> playAttachment;

    // Management buttons
    juce::TextButton clearButton;
    juce::TextButton undoButton;

    // Monitoring button
    juce::TextButton monitorButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> monitorAttachment;

    // Labels
    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::Label loopCountLabel;

    void setupComponents();
    void updateStatusLabel();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LooperView)
};
