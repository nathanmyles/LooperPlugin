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

#include "TrackManager.h"
#include "Track.h"

TrackManager::TrackManager() {}

TrackManager::~TrackManager() {}

void TrackManager::prepare(double sampleRate) {
  const std::lock_guard<std::mutex> lock(tracksMutex);
  currentSampleRate = sampleRate;
  maxLoopLength = static_cast<int>(sampleRate * 60.0);
  baseLoopLength.store(0);
  readPosition.store(0);

  for (auto &track : tracks) {
    track->prepare(sampleRate);
  }
}

void TrackManager::setBaseLoopLength(int length) {
  baseLoopLength.store(length);
}

int TrackManager::getBaseLoopLength() const { return baseLoopLength.load(); }

void TrackManager::incrementReadPosition(int samples) {
  int newPos = readPosition.load() + samples;
  int baseLength = baseLoopLength.load();

  // Wrap around if we've exceeded the base loop length
  if (baseLength > 0 && newPos >= baseLength) {
    newPos = newPos % baseLength;
  }

  readPosition.store(newPos);
}

int TrackManager::getWrappedReadPosition() const {
  int pos = readPosition.load();
  int baseLength = baseLoopLength.load();

  if (baseLength > 0) {
    return pos % baseLength;
  }

  return pos;
}

bool TrackManager::wouldExceedLoopLength(int position) const {
  int baseLength = baseLoopLength.load();
  return baseLength > 0 && position >= baseLength;
}

// Track Management

Track *TrackManager::addTrack() {
  const std::lock_guard<std::mutex> lock(tracksMutex);
  auto track = std::make_unique<Track>(nextTrackId++, *this);
  track->prepare(currentSampleRate);

  Track *trackPtr = track.get();
  tracks.push_back(std::move(track));
  return trackPtr;
}

void TrackManager::removeTrack(int trackId) {
  const std::lock_guard<std::mutex> lock(tracksMutex);
  auto it = std::find_if(tracks.begin(), tracks.end(),
                         [trackId](const std::unique_ptr<Track> &t) {
                           return t->getId() == trackId;
                         });

  if (it != tracks.end()) {
    tracks.erase(it);

    // Check if any tracks still have content
    bool hasContent = false;
    for (const auto &track : tracks) {
      if (track->getLooper().hasLoops()) {
        hasContent = true;
        break;
      }
    }

    if (!hasContent) {
      resetBaseLoopLength();
    }
  }
}

void TrackManager::removeAllTracks() {
  const std::lock_guard<std::mutex> lock(tracksMutex);
  tracks.clear();
  resetBaseLoopLength();
  nextTrackId = 0;
}

std::vector<Track *> TrackManager::getTrackCopies() const {
  const std::lock_guard<std::mutex> lock(tracksMutex);
  std::vector<Track *> copies;
  copies.reserve(tracks.size());
  for (auto &track : tracks) {
    copies.push_back(track.get());
  }
  return copies;
}

int TrackManager::getTrackCount() const {
  const std::lock_guard<std::mutex> lock(tracksMutex);
  return static_cast<int>(tracks.size());
}

Track *TrackManager::findTrack(int trackId) {
  const std::lock_guard<std::mutex> lock(tracksMutex);
  return findTrackInternal(trackId);
}

Track *TrackManager::findTrackInternal(int trackId) const {
  auto it = std::find_if(tracks.begin(), tracks.end(),
                         [trackId](const std::unique_ptr<Track> &t) {
                           return t->getId() == trackId;
                         });

  return (it != tracks.end()) ? it->get() : nullptr;
}

// Track Controls

bool TrackManager::startRecordingTrack(int trackId) {
  const std::lock_guard<std::mutex> lock(tracksMutex);
  Track *track = findTrackInternal(trackId);
  if (track != nullptr) {
    // Stop any other track that's recording (only one at a time)
    stopAllRecordingInternal();

    track->startRecording();

    if (!track->isPlaying()) {
      startPlaybackTrackInternal(trackId);
      return true;
    }
  }
  return false;
}

void TrackManager::stopRecordingTrack(int trackId) {
  const std::lock_guard<std::mutex> lock(tracksMutex);
  Track *track = findTrackInternal(trackId);
  if (track != nullptr) {
    track->stopRecording();
  }
}

void TrackManager::stopAllRecording() {
  const std::lock_guard<std::mutex> lock(tracksMutex);
  stopAllRecordingInternal();
}

void TrackManager::stopAllRecordingInternal() {
  for (auto &track : tracks) {
    if (track->isRecording()) {
      track->stopRecording();
    }
  }
}

void TrackManager::startPlaybackTrack(int trackId) {
  const std::lock_guard<std::mutex> lock(tracksMutex);
  startPlaybackTrackInternal(trackId);
}

void TrackManager::startPlaybackTrackInternal(int trackId) {
  Track *track = findTrackInternal(trackId);
  if (track != nullptr) {
    bool wasAnyPlaying = isPlayingInternal();
    track->startPlayback();
    if (!wasAnyPlaying) {
      resetReadPosition();
    }
  }
}

void TrackManager::stopPlaybackTrack(int trackId) {
  const std::lock_guard<std::mutex> lock(tracksMutex);
  Track *track = findTrackInternal(trackId);
  if (track != nullptr) {
    track->stopPlayback();
  }
}

void TrackManager::clearTrack(int trackId) {
  const std::lock_guard<std::mutex> lock(tracksMutex);
  Track *track = findTrackInternal(trackId);
  if (track != nullptr) {
    track->requestClearAll();

    // Check if any tracks still have content
    bool hasContent = false;
    for (const auto &t : tracks) {
      if (t->getLooper().hasLoops()) {
        hasContent = true;
        break;
      }
    }

    if (!hasContent) {
      resetBaseLoopLength();
    }
  }
}

void TrackManager::undoTrack(int trackId) {
  const std::lock_guard<std::mutex> lock(tracksMutex);
  Track *track = findTrackInternal(trackId);
  if (track != nullptr) {
    track->requestUndoLast();
  }
}

// Global Controls

void TrackManager::requestClearAll() {
  const std::lock_guard<std::mutex> lock(tracksMutex);
  // Stop all recording first so we don't write into freshly-emptied loops
  stopAllRecordingInternal();
  for (auto &track : tracks) {
    track->requestClearAll();
  }
  resetBaseLoopLength();
  resetReadPosition();
}

void TrackManager::requestUndoLast() {
  const std::lock_guard<std::mutex> lock(tracksMutex);
  // Find the track with the most recent loop and undo it
  Track *track = findTrackWithMostRecentLoopInternal();
  if (track != nullptr) {
    // Stop recording on this track to prevent writing into a just-removed loop
    stopAllRecordingInternal();
    track->requestUndoLast();
  }
}

void TrackManager::startPlayback() {
  const std::lock_guard<std::mutex> lock(tracksMutex);
  bool wasAnyPlaying = isPlayingInternal();
  for (auto &track : tracks) {
    track->startPlayback();
  }
  if (!wasAnyPlaying) {
    resetReadPosition();
  }
}

void TrackManager::stopPlayback() {
  const std::lock_guard<std::mutex> lock(tracksMutex);
  for (auto &track : tracks) {
    track->stopPlayback();
  }
}

bool TrackManager::isPlaying() const {
  const std::lock_guard<std::mutex> lock(tracksMutex);
  return isPlayingInternal();
}

bool TrackManager::isPlayingInternal() const {
  for (const auto &track : tracks) {
    if (track->isPlaying()) {
      return true;
    }
  }
  return false;
}

bool TrackManager::isAnyTrackSoloed() const {
  const std::lock_guard<std::mutex> lock(tracksMutex);
  return isAnyTrackSoloedInternal();
}

bool TrackManager::isAnyTrackSoloedInternal() const {
  for (const auto &track : tracks) {
    if (track->isSoloed()) {
      return true;
    }
  }
  return false;
}

void TrackManager::processBlock(juce::AudioBuffer<float> &buffer,
                                bool shouldMonitor) {
  const std::lock_guard<std::mutex> lock(tracksMutex);

  // Handle pending requests for all tracks
  for (auto &track : tracks) {
    track->handlePendingRequests();
  }

  // Check if any track is soloed
  bool anySoloed = isAnyTrackSoloedInternal();

  int loopLen = getBaseLoopLength();

  // First, handle recording for any track that's currently recording
  for (auto &track : tracks) {
    if (track->isRecording()) {
      int maxRecordLen = loopLen > 0 ? loopLen : maxLoopLength;
      track->getLooper().processRecording(buffer, maxRecordLen);
    }
  }

  // If not monitoring, clear the buffer before mixing
  if (!shouldMonitor) {
    buffer.clear();
  }

  // Mix all track outputs
  int currentReadPos = getWrappedReadPosition();
  for (auto &track : tracks) {
    if (track->shouldOutput(anySoloed)) {
      float effectiveVolume = track->getEffectiveVolume(anySoloed);
      track->getLooper().processPlayback(buffer, effectiveVolume,
                                         currentReadPos, loopLen);
    }
  }

  // Update time manager read position for synchronized playback
  if (isPlayingInternal() && loopLen > 0) {
    incrementReadPosition(buffer.getNumSamples());
  }
}

Track *TrackManager::findTrackWithMostRecentLoopInternal() const {
  // For simplicity, find the track with the most loops
  Track *result = nullptr;
  size_t maxLoops = 0;

  for (const auto &track : tracks) {
    size_t loopCount = track->getLooper().getNumLoops();
    if (loopCount > maxLoops) {
      maxLoops = loopCount;
      result = track.get();
    }
  }

  return result;
}

void TrackManager::getState(juce::ValueTree &state, double sampleRate) const {
  const std::lock_guard<std::mutex> lock(tracksMutex);
  state.setProperty("baseLoopLength", getBaseLoopLength(), nullptr);
  state.setProperty("trackCount", static_cast<int>(tracks.size()), nullptr);

  for (size_t i = 0; i < tracks.size(); ++i) {
    juce::ValueTree trackState("Track" + juce::String(i));
    trackState.setProperty("trackId", tracks[i]->getId(), nullptr);
    trackState.setProperty("volume", tracks[i]->getVolume(), nullptr);
    trackState.setProperty("soloed", tracks[i]->isSoloed(), nullptr);

    tracks[i]->getLooper().getState(trackState, sampleRate);

    state.addChild(trackState, -1, nullptr);
  }
}

void TrackManager::setState(const juce::ValueTree &state, double sampleRate) {
  const std::lock_guard<std::mutex> lock(tracksMutex);
  // Restore time manager
  int baseLength = state.getProperty("baseLoopLength", 0);
  if (baseLength > 0) {
    setBaseLoopLength(baseLength);
  }

  // Clear existing tracks
  tracks.clear();
  nextTrackId = 0;

  // Restore tracks
  int trackCount = state.getProperty("trackCount", 0);
  for (int i = 0; i < trackCount; ++i) {
    juce::ValueTree trackState =
        state.getChildWithName("Track" + juce::String(i));
    if (trackState.isValid()) {
      int trackId = trackState.getProperty("trackId", i);
      auto track = std::make_unique<Track>(trackId, *this);
      track->prepare(sampleRate);

      // Restore track properties
      track->setVolume(trackState.getProperty("volume", 0.7f));
      track->setSoloed(trackState.getProperty("soloed", false));

      // Restore looper state
      track->getLooper().setState(trackState, sampleRate);

      tracks.push_back(std::move(track));

      // Update nextTrackId to be higher than any existing track
      if (trackId >= nextTrackId) {
        nextTrackId = trackId + 1;
      }
    }
  }
}
