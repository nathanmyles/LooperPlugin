#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class LooperAudioProcessor  : public juce::AudioProcessor,
                              public juce::Timer
{
public:
    struct Loop {
        juce::AudioBuffer<float> buffer;
        int length = 0;
        bool hasContent = false;
    };

    LooperAudioProcessor();
    ~LooperAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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
    const std::vector<std::unique_ptr<Loop>>& getLoops() const { return loops; }
    
    // Thread-safe methods for UI actions
    void requestClearAll();
    void requestUndoLast();
    
private:
    std::atomic<float>* volumeParam = nullptr;
    std::atomic<float>* recordParam = nullptr;
    std::atomic<float>* playParam = nullptr;

    std::vector<std::unique_ptr<Loop>> loops;
    int baseLoopLength = 0;
    int recordingLoopIndex = -1;
    std::atomic<int> writePosition;
    std::atomic<int> readPosition;
    bool isPlaying;
    double currentSampleRate;
    int maxLoopLength;
    
    std::atomic<bool> requestStopRecording { false };
    std::atomic<bool> requestClear { false };
    std::atomic<bool> requestUndo { false };

    void stopRecording(int loopIndex);
    void addNewLoop();
    void removeLastLoop();

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Timer callback for message thread updates
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperAudioProcessor)
};