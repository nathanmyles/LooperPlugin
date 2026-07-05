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

#include "LoopWaveform.h"

LoopWaveform::LoopWaveform(Track &t) : track(t) {}

LoopWaveform::~LoopWaveform() {}

void LoopWaveform::paint(juce::Graphics &g) {
  auto bounds = getLocalBounds();
  float w = static_cast<float>(bounds.getWidth());
  float h = static_cast<float>(bounds.getHeight());
  float centerY = bounds.getCentreY();

  // Background
  g.setColour(juce::Colours::black.withAlpha(0.35f));
  g.fillRect(bounds);

  auto &looper = track.getLooper();
  int baseLen = track.getBaseLoopLength();

  // During first recording, assume a length and grow it as needed
  int displayLen = baseLen;
  if (displayLen <= 0) {
    if (track.isRecording()) {
      if (assumedLength <= 0) {
        assumedLength = static_cast<int>(looper.getSampleRate() * 8.0);
      }
      int writePos = looper.getRecordingLength();
      while (writePos > assumedLength) {
        assumedLength = static_cast<int>(assumedLength * 1.2);
      }
      displayLen = assumedLength;
    } else {
      assumedLength = 0;
      g.setColour(juce::Colours::grey);
      g.drawText("No loop", bounds, juce::Justification::centred, false);
      return;
    }
  } else {
    assumedLength = 0;
  }

  // Center line
  g.setColour(juce::Colours::darkgrey);
  g.drawHorizontalLine(static_cast<int>(centerY), 0.0f, w);

  // Get waveform peaks using the display length
  int numBins = juce::jmax(1, bounds.getWidth());
  auto peaks = looper.getWaveformPeaks(numBins, 0, displayLen);

  // Build filled waveform path
  juce::Path path;
  float ampScale = h * 0.48f;

  path.startNewSubPath(0.0f, centerY);

  for (int x = 0; x < numBins; ++x) {
    float boosted = juce::jmin(1.0f, peaks[static_cast<size_t>(x)] * 2.5f);
    float y = centerY - boosted * ampScale;
    path.lineTo(static_cast<float>(x), y);
  }

  path.lineTo(w - 1.0f, centerY);

  for (int x = numBins - 1; x >= 0; --x) {
    float boosted = juce::jmin(1.0f, peaks[static_cast<size_t>(x)] * 2.5f);
    float y = centerY + boosted * ampScale;
    path.lineTo(static_cast<float>(x), y);
  }

  path.closeSubPath();

  // Fill waveform
  g.setColour(juce::Colours::cyan.withAlpha(0.45f));
  g.fillPath(path);

  // Outline waveform
  g.setColour(juce::Colours::cyan.withAlpha(0.8f));
  g.strokePath(path, juce::PathStrokeType(1.0f));

  // Playhead (only show once we have a real loop length)
  if (baseLen > 0 && looper.isPlaying()) {
    float playX = static_cast<float>(track.getReadPosition()) / baseLen * w;
    g.setColour(juce::Colours::orange);
    g.drawVerticalLine(static_cast<int>(playX), bounds.getY(),
                       bounds.getBottom());
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.drawVerticalLine(static_cast<int>(playX), centerY - 1.0f, centerY + 1.0f);
  }

  // Recording position (only show once we have a real loop length)
  if (baseLen > 0 && track.isRecording()) {
    float recX = static_cast<float>(track.getReadPosition()) / baseLen * w;
    g.setColour(juce::Colours::red.withAlpha(0.8f));
    g.drawVerticalLine(static_cast<int>(recX), bounds.getY(),
                       bounds.getBottom());
  }
}
