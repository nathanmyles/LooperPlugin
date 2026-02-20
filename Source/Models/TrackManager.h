#pragma once

#include <atomic>
#include <juce_core/juce_core.h>

/**
 * TrackManager - Centralized timing management for multi-track looper
 *
 * Manages the shared base loop length and read position that all tracks
 * synchronize to. Ensures all tracks stay locked together.
 */
class TrackManager
{
public:
    TrackManager();
    ~TrackManager();

    // Initialize with sample rate
    void prepare(double sampleRate);

    // Base loop length management (set by first track to record)
    void setBaseLoopLength(int length);
    int getBaseLoopLength() const;
    bool hasBaseLoopLength() const { return baseLoopLength.load() > 0; }
    void resetBaseLoopLength() { baseLoopLength.store(0); }

    // Shared read position for synchronized playback
    int getReadPosition() const { return readPosition.load(); }
    void setReadPosition(int position) { readPosition.store(position); }
    void incrementReadPosition(int samples);
    void resetReadPosition() { readPosition.store(0); }

    // Calculate wrapped position within base loop length
    int getWrappedReadPosition() const;

    // Check if a position would exceed the base loop length
    bool wouldExceedLoopLength(int position) const;

    // Get max loop duration in samples (e.g., 60 seconds)
    int getMaxLoopLength() const { return maxLoopLength; }

private:
    std::atomic<int> baseLoopLength{0};
    std::atomic<int> readPosition{0};

    double currentSampleRate = 44100.0;
    int maxLoopLength = 44100 * 60;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackManager)
};
