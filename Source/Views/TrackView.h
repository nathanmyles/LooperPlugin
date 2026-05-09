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
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * TrackView - UI component for a single track
 *
 * Displays controls for:
 * - Track name/label
 * - Volume slider
 * - Record button
 * - Mute button
 * - Solo button
 * - Remove button
 * - Loop count display
 */
class TrackView : public juce::Component {
public:
  // Callbacks
  std::function<void(int)> onRemoveTrack;
  std::function<void(int, bool)> onRecordClicked;
  std::function<void(int, bool)> onPlayClicked;
  std::function<void(int)> onClearTrack;
  std::function<void(int)> onUndoTrack;

  TrackView(int trackId, Track &track);
  ~TrackView() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  // Update UI to reflect track state
  void updateFromTrack();

  // Refresh the loop count display
  void refreshLoopCount();

  // Update button styles to reflect current state
  void updateButtonStyles();

  int getTrackId() const { return trackId; }

private:
  int trackId;
  Track &track;

  // UI Components
  juce::Label trackNameLabel;
  juce::Slider volumeSlider;
  juce::TextButton recordButton;
  juce::TextButton soloButton;
  juce::TextButton clearButton;
  juce::TextButton undoButton;
  juce::TextButton removeButton;
  juce::Label loopCountLabel;
  juce::TextButton playButton;

  // State tracking
  bool wasRecording = false;
  int lastLoopCount = 0;

  void setupComponents();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackView)
};
