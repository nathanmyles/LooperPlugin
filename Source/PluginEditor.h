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

#include "PluginProcessor.h"
#include "Views/GlobalControlBar.h"
#include "Views/TrackContainer.h"

class LooperAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   public juce::Timer {
public:
  LooperAudioProcessorEditor(LooperAudioProcessor &);
  ~LooperAudioProcessorEditor() override;

  void paint(juce::Graphics &) override;
  void resized() override;

  // Timer callback for UI updates
  void timerCallback() override;

private:
  LooperAudioProcessor &audioProcessor;

  // UI Components
  GlobalControlBar controlBar;
  TrackContainer trackContainer;

  void setupCallbacks();
  void addInitialTrack();
  void updateTrackButtons();
  void syncTracksWithProcessor();
  bool keyPressed(const juce::KeyPress &key) override;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LooperAudioProcessorEditor)
};
