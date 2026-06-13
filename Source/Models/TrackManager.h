/*
 * LooperPlugin
 * Copyright (C) 2026 NathanMyles
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <atomic>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <memory>
#include <mutex>
#include <vector>

class Track;

/**
 * TrackManager - Centralized management for multi-track looper
 *
 * Manages the collection of tracks and shared timing:
 * - Base loop length and read position that all tracks synchronize to
 * - Track lifecycle (add, remove, find)
 * - Recording/playback controls across all tracks
 * - Audio processing for all tracks
 */
class TrackManager {
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

  // Track management
  Track *addTrack();
  void removeTrack(int trackId);
  void removeAllTracks();
  std::vector<Track *> getTrackCopies() const;
  Track *findTrack(int trackId);
  int getTrackCount() const;

  // Track controls
  bool startRecordingTrack(int trackId);
  void stopRecordingTrack(int trackId);
  void stopAllRecording();
  void startPlaybackTrack(int trackId);
  void stopPlaybackTrack(int trackId);
  void clearTrack(int trackId);
  void undoTrack(int trackId);

  // Global controls
  void requestClearAll();
  void requestUndoLast();
  void startPlayback();
  void stopPlayback();
  bool isPlaying() const;

  // Solo logic
  bool isAnyTrackSoloed() const;

  // Audio processing
  void processBlock(juce::AudioBuffer<float> &buffer, bool shouldMonitor);

  // State serialization
  void getState(juce::ValueTree &state, double sampleRate) const;
  void setState(const juce::ValueTree &state, double sampleRate);

private:
  mutable std::mutex tracksMutex;

  std::atomic<int> baseLoopLength{0};
  std::atomic<int> readPosition{0};

  double currentSampleRate = 44100.0;
  int maxLoopLength = 44100 * 60;

  std::vector<std::unique_ptr<Track>> tracks;
  int nextTrackId = 0;

  // Internal unlocked helpers (caller must hold tracksMutex)
  Track *findTrackInternal(int trackId) const;
  Track *findTrackWithMostRecentLoopInternal() const;
  void stopAllRecordingInternal();
  void startPlaybackTrackInternal(int trackId);
  bool isAnyTrackSoloedInternal() const;
  bool isPlayingInternal() const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackManager)
};
