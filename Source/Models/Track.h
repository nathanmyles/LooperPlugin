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

#include "Looper.h"
#include <atomic>

class TrackManager;

/**
 * Track - A single track in the multi-track looper
 *
 * Wraps a Looper instance and provides per-track controls:
 * - Volume
 * - Mute
 * - Solo
 * - Record (only one track can record at a time)
 */
class Track {
public:
  Track(int trackId, TrackManager &trackManager);
  ~Track();

  // Initialize the track
  void prepare(double sampleRate);

  // Track controls
  void startRecording();
  void stopRecording();
  bool isRecording() const { return recording; }

  void startPlayback();
  void stopPlayback();
  bool isPlaying() const { return playing; }

  // Volume control (0.0 to 1.0)
  void setVolume(float vol) { volume.store(juce::jlimit(0.0f, 1.0f, vol)); }
  float getVolume() const { return volume.load(); }

  // Solo control
  void setSoloed(bool solo) { soloed.store(solo); }
  bool isSoloed() const { return soloed.load(); }

  // Access the underlying looper
  Looper &getLooper() { return looper; }
  const Looper &getLooper() const { return looper; }

  // Track identification
  int getId() const { return trackId; }
  juce::String getName() const { return "Track " + juce::String(trackId + 1); }

  int getReadPosition() const;
  int getBaseLoopLength() const;

  // Clear and undo
  void clearAll();
  void undoLast();
  void requestClearAll() { looper.requestClearAll(); }
  void requestUndoLast() { looper.requestUndoLast(); }
  void handlePendingRequests() { looper.handlePendingRequests(); }

  // Process audio for this track
  // Returns true if this track should contribute to output
  bool shouldOutput(bool anyTrackSoloed) const;

  // Get effective volume considering mute/solo state
  float getEffectiveVolume(bool anyTrackSoloed) const;

private:
  int trackId;
  TrackManager &trackManager;
  Looper looper;

  std::atomic<float> volume{0.7f};
  std::atomic<bool> soloed{false};
  std::atomic<bool> recording{false};
  std::atomic<bool> playing{false};

  double currentSampleRate = 44100.0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Track)
};
