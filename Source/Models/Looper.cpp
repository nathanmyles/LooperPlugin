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

#include "Looper.h"

Looper::Looper() {}

Looper::~Looper() {}

void Looper::prepare(double sampleRate) {
  currentSampleRate = sampleRate;
  maxLoopLength = static_cast<int>(sampleRate * 60.0);

  std::lock_guard<std::mutex> lock(loopsMutex);
  loops.clear();
  baseLoopLength = 0;
  recordingLoopIndex = -1;
  writePosition = 0;
  readPosition = 0;
  playing = false;
}

void Looper::startRecording(int currentReadPosition) {
  std::lock_guard<std::mutex> lock(loopsMutex);
  addNewLoop();
  recordingLoopIndex = static_cast<int>(loops.size()) - 1;

  // Capture the current playback position as the start offset
  // This is where in the base loop the overdub will be placed
  if (baseLoopLength > 0) {
    loops[recordingLoopIndex]->startOffset =
        currentReadPosition % baseLoopLength;
  } else {
    loops[recordingLoopIndex]->startOffset = 0;
  }

  writePosition = 0;
}

void Looper::stopRecording() {
  std::lock_guard<std::mutex> lock(loopsMutex);
  if (recordingLoopIndex != -1) {
    auto &loop = loops[static_cast<size_t>(recordingLoopIndex)];
    int currentWritePos = writePosition.load();

    if (currentWritePos > 0) {
      loop->hasContent = true;

      // If this is the first loop, set base length
      if (baseLoopLength == 0) {
        baseLoopLength = currentWritePos;
      }

      // Pad loop to base length so all loops are the same length
      loop->length = baseLoopLength;

      // Apply Crossfade
      applyCrossfade(recordingLoopIndex);
    }
  }
  recordingLoopIndex = -1;
  requestStopRecording.store(true);
}

void Looper::startPlayback() {
  playing = true;
  readPosition = 0;
}

void Looper::stopPlayback() { playing = false; }

void Looper::addNewLoop() {
  auto newLoop = std::make_unique<Loop>();
  newLoop->buffer.setSize(numChannels, maxLoopLength);
  newLoop->buffer.clear();
  newLoop->length = 0;
  newLoop->hasContent = false;
  loops.push_back(std::move(newLoop));
}

void Looper::removeLastLoop() {
  if (!loops.empty()) {
    // If currently recording this loop, cancel recording
    if (recordingLoopIndex == static_cast<int>(loops.size()) - 1) {
      recordingLoopIndex = -1;
      writePosition = 0;
      requestStopRecording.store(true);
    }
    loops.pop_back();

    // If we removed the last loop, reset base length
    if (loops.empty()) {
      baseLoopLength = 0;
    }
  }
}

void Looper::clearAll() {
  loops.clear();
  baseLoopLength = 0;
  recordingLoopIndex = -1;
  writePosition = 0;
  readPosition = 0;
}

void Looper::processRecording(const juce::AudioBuffer<float> &inputBuffer) {
  if (recordingLoopIndex == -1)
    return;

  const int numSamples = inputBuffer.getNumSamples();

  auto &currentLoop = loops[static_cast<size_t>(recordingLoopIndex)];
  int maxRecordLength = (baseLoopLength > 0) ? baseLoopLength : maxLoopLength;
  int samplesToRecord =
      juce::jmin(numSamples, maxRecordLength - writePosition.load());

  if (samplesToRecord > 0) {
    for (int channel = 0; channel < numChannels; ++channel) {
      currentLoop->buffer.copyFrom(channel, writePosition.load(), inputBuffer,
                                   channel, 0, samplesToRecord);
    }

    writePosition += samplesToRecord;
    currentLoop->length = writePosition.load();

    // If we've hit the base loop length or max, stop
    if (writePosition.load() >= maxRecordLength && baseLoopLength > 0) {
      requestStopRecording.store(true);
    } else if (writePosition.load() >= maxLoopLength) {
      requestStopRecording.store(true);
    }
  }
}

void Looper::processPlayback(juce::AudioBuffer<float> &outputBuffer,
                             float volume, int sharedReadPosition) {
  if (!playing)
    return;

  const int numSamples = outputBuffer.getNumSamples();

  // Lock loops for reading to prevent race condition
  std::lock_guard<std::mutex> lock(loopsMutex);
  bool loopsEmpty = loops.empty();

  if (loopsEmpty)
    return;

  for (int sample = 0; sample < numSamples; ++sample) {
    // Use shared read position from TrackManager, wrapped to base loop length
    int readPos = sharedReadPosition + sample;
    if (baseLoopLength > 0 && readPos >= baseLoopLength) {
      readPos = readPos % baseLoopLength;
    }

    for (int channel = 0; channel < numChannels; ++channel) {
      float mixedSample = 0.0f;

      // Mix all loops
      for (auto &loop : loops) {
        if (loop->hasContent) {
          // Calculate position within this loop, accounting for start offset
          // readPos is the global position, we need to find where we are
          // relative to when this loop started recording
          int effectivePos =
              (readPos - loop->startOffset + baseLoopLength) % baseLoopLength;

          // Only play if we're within the recorded portion of this loop
          if (effectivePos < loop->length) {
            mixedSample += loop->buffer.getSample(channel, effectivePos);
          }
        }
      }

      outputBuffer.addSample(channel, sample, mixedSample * volume);
    }
  }
}

void Looper::applyCrossfade(int loopIndex) {
  if (loopIndex < 0 || loopIndex >= static_cast<int>(loops.size()))
    return;

  auto &loop = loops[static_cast<size_t>(loopIndex)];

  // Apply Crossfade (approx 10ms)
  int fadeSamples = juce::jmin(loop->length, (int)(currentSampleRate * 0.01));

  if (fadeSamples > 0) {
    for (int channel = 0; channel < loop->buffer.getNumChannels(); ++channel) {
      auto *channelData = loop->buffer.getWritePointer(channel);
      for (int i = 0; i < fadeSamples; ++i) {
        float alpha = (float)i / (float)fadeSamples;
        int endSampleIdx = loop->length - fadeSamples + i;
        // Mix start of loop into end of loop
        channelData[endSampleIdx] =
            channelData[endSampleIdx] * (1.0f - alpha) + channelData[i] * alpha;
      }
    }
  }
}

void Looper::requestClearAll() { requestClear.store(true); }

void Looper::requestUndoLast() { requestUndo.store(true); }

void Looper::handlePendingRequests() {
  if (requestClear.exchange(false)) {
    // Lock loops for modification to prevent race condition
    std::lock_guard<std::mutex> lock(loopsMutex);
    clearAll();
  }

  if (requestUndo.exchange(false)) {
    // Lock loops for modification to prevent race condition
    std::lock_guard<std::mutex> lock(loopsMutex);
    removeLastLoop();
  }
}

void Looper::getState(juce::ValueTree &state, double sampleRate) const {
  juce::ignoreUnused(sampleRate);

  state.setProperty("loopCount", static_cast<int>(loops.size()), nullptr);
  state.setProperty("baseLoopLength", baseLoopLength, nullptr);

  // Save each loop's audio data
  for (size_t i = 0; i < loops.size(); ++i) {
    juce::String loopKey = "loop_" + juce::String(i);
    auto &loop = loops[i];

    juce::MemoryBlock loopData;
    juce::MemoryOutputStream loopStream(loopData, true);

    // Write loop metadata
    loopStream.writeInt(loop->length);
    loopStream.writeInt(loop->startOffset);
    loopStream.writeBool(loop->hasContent);

    // Write audio data
    if (loop->hasContent && loop->length > 0) {
      for (int channel = 0; channel < loop->buffer.getNumChannels();
           ++channel) {
        loopStream.write(loop->buffer.getReadPointer(channel),
                         sizeof(float) * static_cast<size_t>(loop->length));
      }
    }

    state.setProperty(loopKey, loopData.toBase64Encoding(), nullptr);
  }
}

void Looper::setState(const juce::ValueTree &state, double sampleRate) {
  // Restore loops
  int loopCount = state.getProperty("loopCount", 0);
  baseLoopLength = state.getProperty("baseLoopLength", 0);

  currentSampleRate = sampleRate;
  maxLoopLength = static_cast<int>(sampleRate * 60.0);

  loops.clear();

  for (int i = 0; i < loopCount; ++i) {
    juce::String loopKey = "loop_" + juce::String(i);
    juce::String loopDataBase64 = state.getProperty(loopKey, "");

    if (loopDataBase64.isNotEmpty()) {
      juce::MemoryBlock loopData;
      loopData.fromBase64Encoding(loopDataBase64);
      juce::MemoryInputStream loopStream(loopData, false);

      auto newLoop = std::make_unique<Loop>();
      newLoop->buffer.setSize(numChannels, maxLoopLength);
      newLoop->buffer.clear();

      newLoop->length = loopStream.readInt();
      newLoop->startOffset = loopStream.readInt();
      newLoop->hasContent = loopStream.readBool();

      if (newLoop->hasContent && newLoop->length > 0) {
        for (int channel = 0; channel < newLoop->buffer.getNumChannels();
             ++channel) {
          loopStream.read(
              newLoop->buffer.getWritePointer(channel),
              static_cast<int>(sizeof(float) *
                               static_cast<size_t>(newLoop->length)));
        }
      }

      loops.push_back(std::move(newLoop));
    }
  }
}

bool Looper::hasLoops() const {
  std::lock_guard<std::mutex> lock(loopsMutex);
  return !loops.empty();
}

size_t Looper::getNumLoops() const {
  std::lock_guard<std::mutex> lock(loopsMutex);
  return loops.size();
}
