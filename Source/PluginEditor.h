#pragma once

#include "PluginProcessor.h"
#include "LooperView.h"

class LooperAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    LooperAudioProcessorEditor(LooperAudioProcessor&);
    ~LooperAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    LooperAudioProcessor& audioProcessor;
    LooperView looperView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LooperAudioProcessorEditor)
};
