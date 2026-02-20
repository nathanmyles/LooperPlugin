#include "Track.h"

Track::Track(int id, TrackManager& tm)
    : trackId(id), trackManager(tm)
{
}

Track::~Track()
{
}

void Track::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    looper.prepare(sampleRate);
}

void Track::startRecording()
{
    if (!recording.load())
    {
        recording.store(true);
        looper.startRecording();
    }
}

void Track::stopRecording()
{
    if (recording.load())
    {
        recording.store(false);
        looper.stopRecording();

        // If this is the first loop, set the base loop length for all tracks
        int baseLength = looper.getBaseLoopLength();
        if (baseLength > 0 && trackManager.getBaseLoopLength() == 0)
        {
            trackManager.setBaseLoopLength(baseLength);
        }
    }
}

void Track::startPlayback()
{
    playing.store(true);
    looper.startPlayback();
}

void Track::stopPlayback()
{
    playing.store(false);
    looper.stopPlayback();
}

void Track::clearAll()
{
    looper.clearAll();
}

void Track::undoLast()
{
    looper.removeLastLoop();
}

bool Track::shouldOutput(bool anyTrackSoloed) const
{
    // If any track is soloed, only soloed tracks play
    if (anyTrackSoloed)
    {
        return soloed.load() && !muted.load();
    }

    // Otherwise, muted tracks don't play
    return !muted.load();
}

float Track::getEffectiveVolume(bool anyTrackSoloed) const
{
    if (!shouldOutput(anyTrackSoloed))
    {
        return 0.0f;
    }

    return volume.load();
}
