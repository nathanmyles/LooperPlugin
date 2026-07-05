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

#include "../Models/Track.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * LoopWaveform - Visualizes the recorded audio loops for a track
 *
 * Displays:
 * - Combined waveform of all loops in the track
 * - Playhead position when playing
 * - Recording position when recording
 */
class LoopWaveform : public juce::Component {
public:
  LoopWaveform(Track &track);
  ~LoopWaveform() override;

  void paint(juce::Graphics &g) override;

private:
  Track &track;
  int assumedLength = 0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoopWaveform)
};
