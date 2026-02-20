#include "TrackManager.h"

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
