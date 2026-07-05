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

#include "Track.h"
#include "TrackManager.h"

Track::Track(int id, TrackManager &tm) : trackId(id), trackManager(tm) {}

Track::~Track() {}

void Track::prepare(double sampleRate) {
  currentSampleRate = sampleRate;
  looper.prepare(sampleRate);
}

void Track::startRecording() {
  if (!recording.load()) {
    recording.store(true);
    looper.startRecording(trackManager.getReadPosition(),
                          trackManager.getBaseLoopLength());
  }
}

void Track::stopRecording() {
  if (recording.load()) {
    int recordedLength = looper.getRecordingLength();
    recording.store(false);
    looper.stopRecording(trackManager.getBaseLoopLength());

    if (recordedLength > 0 && trackManager.getBaseLoopLength() == 0) {
      trackManager.setBaseLoopLength(recordedLength);
    }
  }
}

void Track::startPlayback() {
  playing.store(true);
  looper.startPlayback();
}

void Track::stopPlayback() {
  playing.store(false);
  looper.stopPlayback();
}

void Track::clearAll() { looper.clearAll(); }

void Track::undoLast() { looper.removeLastLoop(); }

int Track::getReadPosition() const {
  return trackManager.getWrappedReadPosition();
}

int Track::getBaseLoopLength() const {
  return trackManager.getBaseLoopLength();
}

bool Track::shouldOutput(bool anyTrackSoloed) const {
  if (anyTrackSoloed)
    return soloed.load();
  return true;
}

float Track::getEffectiveVolume(bool anyTrackSoloed) const {
  if (!shouldOutput(anyTrackSoloed))
    return 0.0f;
  return volume.load();
}
