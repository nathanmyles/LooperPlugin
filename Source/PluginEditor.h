#pragma once

#include "PluginProcessor.h"

class LooperAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                   public juce::AudioProcessorValueTreeState::Listener
{
public:
    LooperAudioProcessorEditor (LooperAudioProcessor&);
    ~LooperAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // AudioProcessorValueTreeState::Listener
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    
    void updateLoopCount();

private:
    LooperAudioProcessor& audioProcessor;
    
    juce::Slider volumeSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;
    juce::Label volumeLabel;
    
    juce::TextButton recordButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> recordAttachment;

    juce::TextButton playButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> playAttachment;

    juce::TextButton clearButton;
    // No attachment - handled directly via onClick

    juce::TextButton undoButton;
    // No attachment - handled directly via onClick
    
    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::Label loopCountLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperAudioProcessorEditor)
};