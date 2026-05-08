#include "TrackManager.h"
#include "Track.h"

TrackManager::TrackManager()
{
}

TrackManager::~TrackManager()
{
}

void TrackManager::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    maxLoopLength = static_cast<int>(sampleRate * 60.0);
    baseLoopLength.store(0);
    readPosition.store(0);

    for (auto& track : tracks)
    {
        track->prepare(sampleRate);
    }
}

void TrackManager::setBaseLoopLength(int length)
{
    baseLoopLength.store(length);
}

int TrackManager::getBaseLoopLength() const
{
    return baseLoopLength.load();
}

void TrackManager::incrementReadPosition(int samples)
{
    int newPos = readPosition.load() + samples;
    int baseLength = baseLoopLength.load();

    // Wrap around if we've exceeded the base loop length
    if (baseLength > 0 && newPos >= baseLength)
    {
        newPos = newPos % baseLength;
    }

    readPosition.store(newPos);
}

int TrackManager::getWrappedReadPosition() const
{
    int pos = readPosition.load();
    int baseLength = baseLoopLength.load();

    if (baseLength > 0)
    {
        return pos % baseLength;
    }

    return pos;
}

bool TrackManager::wouldExceedLoopLength(int position) const
{
    int baseLength = baseLoopLength.load();
    return baseLength > 0 && position >= baseLength;
}

// Track Management

Track* TrackManager::addTrack()
{
    auto track = std::make_unique<Track>(nextTrackId++, *this);
    track->prepare(currentSampleRate);

    Track* trackPtr = track.get();
    tracks.push_back(std::move(track));
    return trackPtr;
}

void TrackManager::removeTrack(int trackId)
{
    auto it = std::find_if(tracks.begin(), tracks.end(),
        [trackId](const std::unique_ptr<Track>& t) { return t->getId() == trackId; });

    if (it != tracks.end())
    {
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
            resetBaseLoopLength();
        }
    }
}

void TrackManager::removeAllTracks()
{
    tracks.clear();
    resetBaseLoopLength();
    nextTrackId = 0;
}

Track* TrackManager::findTrack(int trackId)
{
    auto it = std::find_if(tracks.begin(), tracks.end(),
        [trackId](const std::unique_ptr<Track>& t) { return t->getId() == trackId; });

    return (it != tracks.end()) ? it->get() : nullptr;
}

// Track Controls

bool TrackManager::startRecordingTrack(int trackId)
{
    Track* track = findTrack(trackId);
    if (track != nullptr)
    {
        // Stop any other track that's recording (only one at a time)
        stopAllRecording();

        track->startRecording();

        if (!track->isPlaying())
        {
            startPlaybackTrack(trackId);
            return true;
        }
    }
    return false;
}

void TrackManager::stopRecordingTrack(int trackId)
{
    Track* track = findTrack(trackId);
    if (track != nullptr)
    {
        track->stopRecording();
    }
}

void TrackManager::stopAllRecording()
{
    for (auto& track : tracks)
    {
        if (track->isRecording())
        {
            track->stopRecording();
        }
    }
}

void TrackManager::startPlaybackTrack(int trackId)
{
    Track* track = findTrack(trackId);
    if (track != nullptr)
    {
        track->startPlayback();
    }
}

void TrackManager::stopPlaybackTrack(int trackId)
{
    Track* track = findTrack(trackId);
    if (track != nullptr)
    {
        track->stopPlayback();
    }
}

void TrackManager::clearTrack(int trackId)
{
    Track* track = findTrack(trackId);
    if (track != nullptr)
    {
        track->requestClearAll();

        // Check if any tracks still have content
        bool hasContent = false;
        for (const auto& t : tracks)
        {
            if (!t->getLooper().getLoops().empty())
            {
                hasContent = true;
                break;
            }
        }

        if (!hasContent)
        {
            resetBaseLoopLength();
        }
    }
}

void TrackManager::undoTrack(int trackId)
{
    Track* track = findTrack(trackId);
    if (track != nullptr)
    {
        track->requestUndoLast();
    }
}

// Global Controls

void TrackManager::requestClearAll()
{
    for (auto& track : tracks)
    {
        track->requestClearAll();
    }
    resetBaseLoopLength();
    resetReadPosition();
}

void TrackManager::requestUndoLast()
{
    // Find the track with the most recent loop and undo it
    Track* track = findTrackWithMostRecentLoop();
    if (track != nullptr)
    {
        track->requestUndoLast();
    }
}

void TrackManager::startPlayback()
{
    for (auto& track : tracks)
    {
        track->startPlayback();
    }
}

void TrackManager::stopPlayback()
{
    for (auto& track : tracks)
    {
        track->stopPlayback();
    }
}

bool TrackManager::isPlaying() const
{
    for (const auto& track : tracks)
    {
        if (track->isPlaying())
        {
            return true;
        }
    }
    return false;
}

bool TrackManager::isAnyTrackSoloed() const
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

void TrackManager::toggleLastTrackRecording()
{
    if (tracks.empty())
        return;

    auto& lastTrack = tracks.back();
    if (lastTrack->isRecording())
    {
        stopAllRecording();
    }
    else
    {
        startRecordingTrack(lastTrack->getId());
    }
}

void TrackManager::processBlock(juce::AudioBuffer<float>& buffer, bool shouldMonitor)
{
    // Handle pending requests for all tracks
    for (auto& track : tracks)
    {
        track->handlePendingRequests();
    }

    // Check if any track is soloed
    bool anySoloed = isAnyTrackSoloed();

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
    int currentReadPos = getWrappedReadPosition();
    for (auto& track : tracks)
    {
        if (track->shouldOutput(anySoloed))
        {
            float effectiveVolume = track->getEffectiveVolume(anySoloed);
            track->getLooper().processPlayback(buffer, effectiveVolume, currentReadPos);
        }
    }

    // Update time manager read position for synchronized playback
    if (isPlaying() && hasBaseLoopLength())
    {
        incrementReadPosition(buffer.getNumSamples());
    }
}

Track* TrackManager::findTrackWithMostRecentLoop() const
{
    // For simplicity, find the track with the most loops
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

void TrackManager::getState(juce::ValueTree& state, double sampleRate) const
{
    state.setProperty("baseLoopLength", getBaseLoopLength(), nullptr);
    state.setProperty("trackCount", static_cast<int>(tracks.size()), nullptr);

    for (size_t i = 0; i < tracks.size(); ++i)
    {
        juce::ValueTree trackState("Track" + juce::String(i));
        trackState.setProperty("trackId", tracks[i]->getId(), nullptr);
        trackState.setProperty("volume", tracks[i]->getVolume(), nullptr);
        trackState.setProperty("muted", tracks[i]->isMuted(), nullptr);
        trackState.setProperty("soloed", tracks[i]->isSoloed(), nullptr);

        tracks[i]->getLooper().getState(trackState, sampleRate);

        state.addChild(trackState, -1, nullptr);
    }
}

void TrackManager::setState(const juce::ValueTree& state, double sampleRate)
{
    // Restore time manager
    int baseLength = state.getProperty("baseLoopLength", 0);
    if (baseLength > 0)
    {
        setBaseLoopLength(baseLength);
    }

    // Clear existing tracks
    tracks.clear();
    nextTrackId = 0;

    // Restore tracks
    int trackCount = state.getProperty("trackCount", 0);
    for (int i = 0; i < trackCount; ++i)
    {
        juce::ValueTree trackState = state.getChildWithName("Track" + juce::String(i));
        if (trackState.isValid())
        {
            int trackId = trackState.getProperty("trackId", i);
            auto track = std::make_unique<Track>(trackId, *this);
            track->prepare(sampleRate);

            // Restore track properties
            track->setVolume(trackState.getProperty("volume", 0.7f));
            track->setMuted(trackState.getProperty("muted", false));
            track->setSoloed(trackState.getProperty("soloed", false));

            // Restore looper state
            track->getLooper().setState(trackState, sampleRate);

            tracks.push_back(std::move(track));

            // Update nextTrackId to be higher than any existing track
            if (trackId >= nextTrackId)
            {
                nextTrackId = trackId + 1;
            }
        }
    }
}
