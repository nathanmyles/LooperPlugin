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
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <mutex>
#include <vector>

/**
 * Looper class - encapsulates audio looping functionality
 *
 * Handles recording multiple loops, synchronizing them to a base length,
 * and mixing them together for playback.
 */
class Looper {
public:
  struct Loop {
    juce::AudioBuffer<float> buffer;
    int length = 0;
    int startOffset = 0; // Where in the base loop this recording started
    bool hasContent = false;
  };

  Looper();
  ~Looper();

  // Initialize the looper with sample rate
  void prepare(double sampleRate);

  void startRecording(int currentReadPosition, int loopLength);
  void stopRecording(int loopLength);
  bool isRecording() const { return recordingLoopIndex != -1; }

  // Playback control
  void startPlayback();
  void stopPlayback();
  bool isPlaying() const { return playing; }

  // Loop management
  void addNewLoop();
  void removeLastLoop();
  void clearAll();

  void processRecording(const juce::AudioBuffer<float> &inputBuffer,
                        int maxRecordLength);
  void processPlayback(juce::AudioBuffer<float> &outputBuffer, float volume,
                       int readPosition, int loopLength);

  // Thread-safe actions (to be called from non-audio thread)
  void requestClearAll();
  void requestUndoLast();
  void handlePendingRequests();

  bool hasLoops() const;
  size_t getNumLoops() const;
  double getSampleRate() const { return currentSampleRate; }

  // Start offset of the currently-recording loop (for syncing write head)
  int getRecordingOffset() const;

  // Total length recorded in the currently-recording loop
  int getRecordingLength() const;

  // Thread-safe waveform peak extraction for visualization
  // Fills `numBins` peak values from the combined loop audio
  // If effectiveLength > 0, it overrides the base loop length for display
  std::vector<float> getWaveformPeaks(int numBins, int channel = 0,
                                      int effectiveLength = 0) const;

  // State serialization
  void getState(juce::ValueTree &state, double sampleRate) const;
  void setState(const juce::ValueTree &state, double sampleRate);

private:
  mutable std::mutex loopsMutex;
  std::vector<std::unique_ptr<Loop>> loops;
  int recordingLoopIndex = -1;
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
