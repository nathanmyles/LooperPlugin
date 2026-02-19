#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Looper.h"

class LooperAudioProcessor  : public juce::AudioProcessor
{
public:
    LooperAudioProcessor();
    ~LooperAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;

public:
    // Allow editor to access loop count for display
    const std::vector<std::unique_ptr<Looper::Loop>>& getLoops() const { return looper.getLoops(); }

    // Thread-safe methods for UI actions
    void requestClearAll();
    void requestUndoLast();

private:
    std::atomic<float>* volumeParam = nullptr;
    std::atomic<float>* recordParam = nullptr;
    std::atomic<float>* playParam = nullptr;
    std::atomic<float>* monitorParam = nullptr;

    Looper looper;
    double currentSampleRate = 44100.0;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperAudioProcessor)
};
