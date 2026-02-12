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
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> clearAttachment;
    
    juce::Label titleLabel;
    juce::Label statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperAudioProcessorEditor)
};