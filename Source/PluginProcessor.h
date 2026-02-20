#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Models/TrackManager.h"
#include "Models/Track.h"

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
    // Track management for editor
    Track* addTrack();
    void removeTrack(int trackId);
    void removeAllTracks();
    std::vector<std::unique_ptr<Track>>& getTracks() { return tracks; }
    Track* findTrack(int trackId);
    int getTrackCount() const { return static_cast<int>(tracks.size()); }

    // Track controls
    void startRecordingTrack(int trackId);
    void stopRecordingTrack(int trackId);
    void stopAllRecording();
    void clearTrack(int trackId);
    void undoTrack(int trackId);

    // Global controls
    void requestClearAll();
    void requestUndoLast();
    void startPlayback();
    void stopPlayback();
    bool isPlaying() const { return isPlayingState; }

    // Solo logic
    bool isAnyTrackSoloed() const;

    // Access to track manager
    TrackManager& getTrackManager() { return trackManager; }

private:
    std::atomic<float>* playParam = nullptr;
    std::atomic<float>* monitorParam = nullptr;

    // Core components
    TrackManager trackManager;
    std::vector<std::unique_ptr<Track>> tracks;

    // State
    double currentSampleRate = 44100.0;
    bool isPlayingState = false;
    int nextTrackId = 0;

    // Helper for finding which track has the most recent loop (for undo)
    Track* findTrackWithMostRecentLoop() const;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperAudioProcessor)
};
