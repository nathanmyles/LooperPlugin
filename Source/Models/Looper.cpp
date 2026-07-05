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
#include <cmath>

Looper::Looper() {}

Looper::~Looper() {}

void Looper::prepare(double sampleRate) {
  currentSampleRate = sampleRate;
  maxLoopLength = static_cast<int>(sampleRate * 60.0);

  std::lock_guard<std::mutex> lock(loopsMutex);
  loops.clear();
  recordingLoopIndex = -1;
  playing = false;
}

void Looper::startRecording(int currentReadPosition, int loopLength) {
  std::lock_guard<std::mutex> lock(loopsMutex);
  addNewLoop();
  recordingLoopIndex = static_cast<int>(loops.size()) - 1;

  if (loopLength > 0) {
    loops[recordingLoopIndex]->startOffset = currentReadPosition % loopLength;
  } else {
    loops[recordingLoopIndex]->startOffset = 0;
  }
}

void Looper::stopRecording(int loopLength) {
  std::lock_guard<std::mutex> lock(loopsMutex);
  if (recordingLoopIndex != -1) {
    auto &loop = loops[static_cast<size_t>(recordingLoopIndex)];

    if (loop->length > 0) {
      loop->hasContent = true;

      loop->length = loopLength > 0 ? loopLength : loop->length;

      applyCrossfade(recordingLoopIndex);
    }
  }
  recordingLoopIndex = -1;
  requestStopRecording.store(true);
}

void Looper::startPlayback() { playing = true; }

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
      requestStopRecording.store(true);
    }
    loops.pop_back();
  }
}

void Looper::clearAll() {
  loops.clear();
  recordingLoopIndex = -1;
}

void Looper::processRecording(const juce::AudioBuffer<float> &inputBuffer,
                              int maxRecordLength) {
  std::lock_guard<std::mutex> lock(loopsMutex);
  if (recordingLoopIndex == -1)
    return;

  const int numSamples = inputBuffer.getNumSamples();

  auto &currentLoop = loops[static_cast<size_t>(recordingLoopIndex)];
  int samplesToRecord =
      juce::jmin(numSamples, maxRecordLength - currentLoop->length);

  if (samplesToRecord > 0) {
    for (int channel = 0; channel < numChannels; ++channel) {
      currentLoop->buffer.copyFrom(channel, currentLoop->length, inputBuffer,
                                   channel, 0, samplesToRecord);
    }

    currentLoop->length += samplesToRecord;

    if (currentLoop->length >= maxRecordLength) {
      requestStopRecording.store(true);
    }
  }
}

void Looper::processPlayback(juce::AudioBuffer<float> &outputBuffer,
                             float volume, int readPosition, int loopLength) {
  if (!playing || loopLength <= 0)
    return;

  const int numSamples = outputBuffer.getNumSamples();

  std::lock_guard<std::mutex> lock(loopsMutex);

  if (loops.empty())
    return;

  for (int sample = 0; sample < numSamples; ++sample) {
    int pos = readPosition + sample;
    if (pos >= loopLength)
      pos = pos % loopLength;

    for (int channel = 0; channel < numChannels; ++channel) {
      float mixedSample = 0.0f;

      for (size_t li = 0; li < loops.size(); ++li) {
        auto &loop = loops[li];
        bool isRecordingLoop = (static_cast<int>(li) == recordingLoopIndex);
        if (!loop->hasContent && !isRecordingLoop)
          continue;

        int effectivePos = (pos - loop->startOffset + loopLength) % loopLength;

        if (effectivePos < loop->length) {
          mixedSample += loop->buffer.getSample(channel, effectivePos);
        }
      }

      outputBuffer.addSample(channel, sample, std::tanh(mixedSample) * volume);
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

      // Copy the fade-in region before writing — the read/write ranges may
      // overlap when loop->length < 2*fadeSamples, corrupting the source.
      std::vector<float> fadeIn(channelData, channelData + fadeSamples);

      for (int i = 0; i < fadeSamples; ++i) {
        float alpha = static_cast<float>(i) / static_cast<float>(fadeSamples);
        int endSampleIdx = loop->length - fadeSamples + i;
        channelData[endSampleIdx] =
            channelData[endSampleIdx] * (1.0f - alpha) + fadeIn[i] * alpha;
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
  int loopCount = state.getProperty("loopCount", 0);

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

int Looper::getRecordingOffset() const {
  std::lock_guard<std::mutex> lock(loopsMutex);
  if (recordingLoopIndex >= 0 &&
      recordingLoopIndex < static_cast<int>(loops.size())) {
    return loops[static_cast<size_t>(recordingLoopIndex)]->startOffset;
  }
  return 0;
}

int Looper::getRecordingLength() const {
  std::lock_guard<std::mutex> lock(loopsMutex);
  if (recordingLoopIndex >= 0 &&
      recordingLoopIndex < static_cast<int>(loops.size())) {
    return loops[static_cast<size_t>(recordingLoopIndex)]->length;
  }
  return 0;
}

std::vector<float> Looper::getWaveformPeaks(int numBins, int channel,
                                            int effectiveLength) const {
  std::lock_guard<std::mutex> lock(loopsMutex);
  std::vector<float> peaks(numBins, 0.0f);

  if (loops.empty()) {
    return peaks;
  }

  if (effectiveLength <= 0) {
    return peaks;
  }
  int effectiveLen = effectiveLength;

  for (size_t i = 0; i < loops.size(); ++i) {
    auto &loop = loops[i];
    bool isRecording = (static_cast<int>(i) == recordingLoopIndex);
    if ((!loop->hasContent && !isRecording) || loop->length <= 0) {
      continue;
    }

    int safeChannel = juce::jmin(channel, loop->buffer.getNumChannels() - 1);

    for (int bin = 0; bin < numBins; ++bin) {
      int globalPos = static_cast<int>(
          (static_cast<int64_t>(bin) * effectiveLen) / numBins);
      int effectivePos =
          (globalPos - loop->startOffset + effectiveLen) % effectiveLen;

      if (effectivePos < loop->length) {
        float samp = loop->buffer.getSample(safeChannel, effectivePos);
        peaks[bin] = juce::jmax(peaks[bin], std::abs(samp));
      }
    }
  }

  return peaks;
}
