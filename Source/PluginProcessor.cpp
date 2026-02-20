#include "PluginProcessor.h"
#include "PluginEditor.h"

LooperAudioProcessor::LooperAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    playParam = parameters.getRawParameterValue ("play");
    monitorParam = parameters.getRawParameterValue ("monitor");
}

LooperAudioProcessor::~LooperAudioProcessor()
{
}

const juce::String LooperAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool LooperAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool LooperAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool LooperAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double LooperAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int LooperAudioProcessor::getNumPrograms()
{
    return 1;
}

int LooperAudioProcessor::getCurrentProgram()
{
    return 0;
}

void LooperAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused(index);
}

const juce::String LooperAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused(index);
    return {};
}

void LooperAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void LooperAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    currentSampleRate = sampleRate;
    trackManager.prepare(sampleRate);

    // Prepare all existing tracks
    for (auto& track : tracks)
    {
        track->prepare(sampleRate);
    }
}

void LooperAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool LooperAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    const auto& mainOutput = layouts.getMainOutputChannelSet();
    const auto& mainInput  = layouts.getMainInputChannelSet();

    if (mainOutput != mainInput)
        return false;

    if (mainOutput.size() > 2)
        return false;

    return true;
  #endif
}
#endif

void LooperAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    // Handle pending requests for all tracks
    for (auto& track : tracks)
    {
        track->handlePendingRequests();
    }

    const auto shouldPlay = playParam->load() > 0.5f;
    const auto shouldMonitor = monitorParam->load() > 0.5f;

    // Handle global play state
    if (shouldPlay && !isPlayingState)
    {
        startPlayback();
    }
    else if (!shouldPlay && isPlayingState)
    {
        stopPlayback();
    }

    // Check if any track is soloed
    bool anySoloed = isAnyTrackSoloed();

    // Process each track
    // First, handle recording for any track that's currently recording
    for (auto& track : tracks)
    {
        if (track->isRecording())
        {
            track->getLooper().processRecording(buffer);
        }
    }

    // If not monitoring, clear the buffer before mixing
    if (!shouldMonitor)
    {
        buffer.clear();
    }

    // Mix all track outputs
    for (auto& track : tracks)
    {
        if (track->shouldOutput(anySoloed))
        {
            float effectiveVolume = track->getEffectiveVolume(anySoloed);
            track->getLooper().processPlayback(buffer, effectiveVolume);
        }
    }

    // Update time manager read position for synchronized playback
    if (isPlayingState && trackManager.hasBaseLoopLength())
    {
        trackManager.incrementReadPosition(buffer.getNumSamples());
    }
}

// Track Management

Track* LooperAudioProcessor::addTrack()
{
    auto track = std::make_unique<Track>(nextTrackId++, trackManager);
    track->prepare(currentSampleRate);

    // If we already have a base loop length, sync this track to it
    if (trackManager.hasBaseLoopLength())
    {
        // Track will sync to the existing base length when it starts recording
    }

    Track* trackPtr = track.get();
    tracks.push_back(std::move(track));
    return trackPtr;
}

void LooperAudioProcessor::removeTrack(int trackId)
{
    auto it = std::find_if(tracks.begin(), tracks.end(),
        [trackId](const std::unique_ptr<Track>& t) { return t->getId() == trackId; });

    if (it != tracks.end())
    {
        // If this was the last track with loops, reset the base loop length
        tracks.erase(it);

        // Check if any tracks still have content
        bool hasContent = false;
        for (const auto& track : tracks)
        {
            if (!track->getLooper().getLoops().empty())
            {
                hasContent = true;
                break;
            }
        }

        if (!hasContent)
        {
            trackManager.resetBaseLoopLength();
        }
    }
}

void LooperAudioProcessor::removeAllTracks()
{
    tracks.clear();
    trackManager.resetBaseLoopLength();
    nextTrackId = 0;
}

Track* LooperAudioProcessor::findTrack(int trackId)
{
    auto it = std::find_if(tracks.begin(), tracks.end(),
        [trackId](const std::unique_ptr<Track>& t) { return t->getId() == trackId; });

    return (it != tracks.end()) ? it->get() : nullptr;
}

// Track Controls

void LooperAudioProcessor::startRecordingTrack(int trackId)
{
    Track* track = findTrack(trackId);
    if (track != nullptr)
    {
        // Stop any other track that's recording (only one at a time)
        stopAllRecording();

        track->startRecording();

        // Auto-start playback if not already playing
        if (!isPlayingState)
        {
            parameters.getParameter("play")->setValueNotifyingHost(1.0f);
        }
    }
}

void LooperAudioProcessor::stopRecordingTrack(int trackId)
{
    Track* track = findTrack(trackId);
    if (track != nullptr)
    {
        track->stopRecording();

        // If this track set the base loop length, sync all tracks
        if (trackManager.getBaseLoopLength() == 0 && track->getLooper().getBaseLoopLength() > 0)
        {
            trackManager.setBaseLoopLength(track->getLooper().getBaseLoopLength());
        }
    }
}

void LooperAudioProcessor::stopAllRecording()
{
    for (auto& track : tracks)
    {
        if (track->isRecording())
        {
            track->stopRecording();
        }
    }
}

void LooperAudioProcessor::clearTrack(int trackId)
{
    Track* track = findTrack(trackId);
    if (track != nullptr)
    {
        track->requestClearAll();
    }
}

void LooperAudioProcessor::undoTrack(int trackId)
{
    Track* track = findTrack(trackId);
    if (track != nullptr)
    {
        track->requestUndoLast();
    }
}

// Global Controls

void LooperAudioProcessor::requestClearAll()
{
    for (auto& track : tracks)
    {
        track->requestClearAll();
    }
    trackManager.resetBaseLoopLength();
}

void LooperAudioProcessor::requestUndoLast()
{
    // Find the track with the most recent loop and undo it
    Track* track = findTrackWithMostRecentLoop();
    if (track != nullptr)
    {
        track->requestUndoLast();
    }
}

void LooperAudioProcessor::startPlayback()
{
    isPlayingState = true;
    for (auto& track : tracks)
    {
        track->startPlayback();
    }
}

void LooperAudioProcessor::stopPlayback()
{
    isPlayingState = false;
    for (auto& track : tracks)
    {
        track->stopPlayback();
    }
}

bool LooperAudioProcessor::isAnyTrackSoloed() const
{
    for (const auto& track : tracks)
    {
        if (track->isSoloed())
        {
            return true;
        }
    }
    return false;
}

Track* LooperAudioProcessor::findTrackWithMostRecentLoop() const
{
    // For simplicity, find the track with the most loops
    // In a more sophisticated version, we'd track timestamps
    Track* result = nullptr;
    size_t maxLoops = 0;

    for (const auto& track : tracks)
    {
        size_t loopCount = track->getLooper().getLoops().size();
        if (loopCount > maxLoops)
        {
            maxLoops = loopCount;
            result = track.get();
        }
    }

    return result;
}

bool LooperAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* LooperAudioProcessor::createEditor()
{
    return new LooperAudioProcessorEditor (*this);
}

void LooperAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree state = parameters.copyState();

    // Save time manager state
    state.setProperty("baseLoopLength", trackManager.getBaseLoopLength(), nullptr);
    state.setProperty("trackCount", static_cast<int>(tracks.size()), nullptr);

    // Save each track's state
    for (size_t i = 0; i < tracks.size(); ++i)
    {
        juce::ValueTree trackState("Track" + juce::String(i));
        trackState.setProperty("trackId", tracks[i]->getId(), nullptr);
        trackState.setProperty("volume", tracks[i]->getVolume(), nullptr);
        trackState.setProperty("muted", tracks[i]->isMuted(), nullptr);
        trackState.setProperty("soloed", tracks[i]->isSoloed(), nullptr);

        // Save looper state
        tracks[i]->getLooper().getState(trackState, currentSampleRate);

        state.addChild(trackState, -1, nullptr);
    }

    juce::MemoryOutputStream stream (destData, true);
    state.writeToStream (stream);
}

void LooperAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ValueTree state = juce::ValueTree::readFromData (data, static_cast<size_t>(sizeInBytes));
    if (state.isValid())
    {
        parameters.replaceState (state);

        // Restore time manager
        int baseLength = state.getProperty("baseLoopLength", 0);
        if (baseLength > 0)
        {
            trackManager.setBaseLoopLength(baseLength);
        }

        // Clear existing tracks
        tracks.clear();

        // Restore tracks
        int trackCount = state.getProperty("trackCount", 0);
        for (int i = 0; i < trackCount; ++i)
        {
            juce::ValueTree trackState = state.getChildWithName("Track" + juce::String(i));
            if (trackState.isValid())
            {
                int trackId = trackState.getProperty("trackId", i);
                auto track = std::make_unique<Track>(trackId, trackManager);
                track->prepare(currentSampleRate);

                // Restore track properties
                track->setVolume(trackState.getProperty("volume", 0.7f));
                track->setMuted(trackState.getProperty("muted", false));
                track->setSoloed(trackState.getProperty("soloed", false));

                // Restore looper state
                track->getLooper().setState(trackState, currentSampleRate);

                tracks.push_back(std::move(track));

                // Update nextTrackId to be higher than any existing track
                if (trackId >= nextTrackId)
                {
                    nextTrackId = trackId + 1;
                }
            }
        }
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout LooperAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Global controls only - per-track controls are managed internally
    layout.add (std::make_unique<juce::AudioParameterBool>("play", "Play", false));
    layout.add (std::make_unique<juce::AudioParameterBool>("monitor", "Monitor", false));

    return layout;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LooperAudioProcessor();
}
