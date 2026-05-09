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

#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * GlobalControlBar - Top-level controls for the multi-track looper
 *
 * Contains:
 * - Play All button (global play)
 * - Stop All button (global stop)
 * - Monitor button (input monitoring)
 * - Clear All button
 * - Undo Last button (on last modified track or global undo)
 * - Status/Info display
 */
class GlobalControlBar : public juce::Component,
                         public juce::AudioProcessorValueTreeState::Listener {
public:
  // Callbacks
  std::function<void(bool)> onPlayChanged;
  std::function<void(bool)> onMonitorChanged;
  std::function<void()> onStopAll;
  std::function<void()> onClearAll;
  std::function<void()> onUndoLast;

  GlobalControlBar(juce::AudioProcessorValueTreeState &parameters);
  ~GlobalControlBar() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  // AudioProcessorValueTreeState::Listener
  void parameterChanged(const juce::String &parameterID,
                        float newValue) override;

  // Update status display
  void setStatusText(const juce::String &text);
  void setLoopInfo(int trackCount, int totalLoops);

  // Set play all button state without triggering callbacks
  void setPlayAllButtonState(bool isPlaying);

private:
  juce::AudioProcessorValueTreeState &parameters;

  // Controls
  juce::TextButton playButton;

  juce::TextButton stopAllButton;

  juce::TextButton monitorButton;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      monitorAttachment;

  juce::TextButton clearAllButton;
  juce::TextButton undoLastButton;

  // Labels
  juce::Label titleLabel;
  juce::Label statusLabel;
  juce::Label infoLabel;

  void setupComponents();
  void updateButtonStyles();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalControlBar)
};
