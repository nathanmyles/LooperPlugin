#pragma once

#include "PluginProcessor.h"

class LooperAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                   public juce::Button::Listener
{
public:
    LooperAudioProcessorEditor (LooperAudioProcessor&);
    ~LooperAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Button::Listener  
    void buttonClicked (juce::Button* button) override;

private:
    LooperAudioProcessor& audioProcessor;
    
    juce::Slider loopLengthSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> loopLengthAttachment;
    juce::Label loopLengthLabel;
    
    juce::TextButton recordButton;
    juce::RangedAudioParameter* recordParam;
    juce::TextButton playButton;
    juce::RangedAudioParameter* playParam;
    juce::TextButton clearButton;
    juce::RangedAudioParameter* clearParam;
    
    juce::Label titleLabel;
    juce::Label statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperAudioProcessorEditor)
};