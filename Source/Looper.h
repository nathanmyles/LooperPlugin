#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <memory>
#include <atomic>

/**
 * Looper class - encapsulates audio looping functionality
 * 
 * Handles recording multiple loops, synchronizing them to a base length,
 * and mixing them together for playback.
 */
class Looper
{
public:
    struct Loop
    {
        juce::AudioBuffer<float> buffer;
        int length = 0;
        bool hasContent = false;
    };

    Looper();
    ~Looper();

    // Initialize the looper with sample rate
    void prepare(double sampleRate);

    // Recording control
    void startRecording();
    void stopRecording();
    bool isRecording() const { return recordingLoopIndex != -1; }

    // Playback control
    void startPlayback();
    void stopPlayback();
    bool isPlaying() const { return playing; }

    // Loop management
    void addNewLoop();
    void removeLastLoop();
    void clearAll();

    // Audio processing
    void processRecording(const juce::AudioBuffer<float>& inputBuffer);
    void processPlayback(juce::AudioBuffer<float>& outputBuffer, float volume);

    // Thread-safe actions (to be called from non-audio thread)
    void requestClearAll();
    void requestUndoLast();
    void handlePendingRequests();

    // Getters
    const std::vector<std::unique_ptr<Loop>>& getLoops() const { return loops; }
    int getBaseLoopLength() const { return baseLoopLength; }
    int getWritePosition() const { return writePosition.load(); }
    int getReadPosition() const { return readPosition.load(); }

    // State serialization
    void getState(juce::ValueTree& state, double sampleRate) const;
    void setState(const juce::ValueTree& state, double sampleRate);

private:
    std::vector<std::unique_ptr<Loop>> loops;
    int baseLoopLength = 0;
    int recordingLoopIndex = -1;
    std::atomic<int> writePosition{0};
    std::atomic<int> readPosition{0};
    bool playing = false;
    
    double currentSampleRate = 44100.0;
    int maxLoopLength = 44100 * 60;
    int numChannels = 2;

    // Crossfade helper
    void applyCrossfade(int loopIndex);

    // Thread-safe request flags
    std::atomic<bool> requestClear{false};
    std::atomic<bool> requestUndo{false};
    std::atomic<bool> requestStopRecording{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Looper)
};
