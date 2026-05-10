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

#include "TrackView.h"

TrackView::TrackView(int id, Track &t) : trackId(id), track(t) {
  setupComponents();
  updateFromTrack();
}

TrackView::~TrackView() {}

void TrackView::setupComponents() {
  // Track name label
  trackNameLabel.setText(track.getName(), juce::dontSendNotification);
  trackNameLabel.setJustificationType(juce::Justification::centred);
  trackNameLabel.setFont(juce::Font(14.0f, juce::Font::bold));
  addAndMakeVisible(trackNameLabel);

  // Volume slider
  volumeSlider.setSliderStyle(juce::Slider::LinearVertical);
  volumeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
  volumeSlider.setRange(0.0, 1.0, 0.01);
  volumeSlider.setValue(track.getVolume());
  volumeSlider.onValueChange = [this]() {
    track.setVolume(static_cast<float>(volumeSlider.getValue()));
  };
  addAndMakeVisible(volumeSlider);

  // Record button
  recordButton.setButtonText("Record");
  recordButton.setColour(juce::TextButton::buttonColourId, juce::Colours::grey);
  recordButton.setColour(juce::TextButton::buttonOnColourId,
                         juce::Colours::red);
  recordButton.setColour(juce::TextButton::textColourOnId,
                         juce::Colours::white);
  recordButton.setClickingTogglesState(true);
  recordButton.onClick = [this]() {
    if (onRecordClicked)
      onRecordClicked(trackId, recordButton.getToggleState());
  };
  addAndMakeVisible(recordButton);

  // Play button
  playButton.setButtonText("Play");
  playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::grey);
  playButton.setColour(juce::TextButton::buttonOnColourId,
                       juce::Colours::green);
  playButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
  playButton.setClickingTogglesState(true);
  playButton.onClick = [this]() {
    if (onPlayClicked)
      onPlayClicked(trackId, playButton.getToggleState());
  };
  addAndMakeVisible(playButton);

  // Solo button
  soloButton.setButtonText("Solo");
  soloButton.setColour(juce::TextButton::buttonColourId, juce::Colours::grey);
  soloButton.setColour(juce::TextButton::buttonOnColourId,
                       juce::Colours::yellow);
  soloButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
  soloButton.setClickingTogglesState(true);
  soloButton.onClick = [this]() {
    track.setSoloed(soloButton.getToggleState());
  };
  addAndMakeVisible(soloButton);

  // Clear button
  clearButton.setButtonText("Clear");
  clearButton.onClick = [this]() {
    if (onClearTrack) {
      onClearTrack(trackId);
    }
  };
  addAndMakeVisible(clearButton);

  // Undo button
  undoButton.setButtonText("Undo");
  undoButton.onClick = [this]() {
    if (onUndoTrack) {
      onUndoTrack(trackId);
    }
  };
  addAndMakeVisible(undoButton);

  // Remove button
  removeButton.setButtonText("X");
  removeButton.onClick = [this]() {
    if (onRemoveTrack) {
      onRemoveTrack(trackId);
    }
  };
  addAndMakeVisible(removeButton);

  // Loop count label
  loopCountLabel.setText("Loops: 0", juce::dontSendNotification);
  loopCountLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(loopCountLabel);

  updateButtonStyles();
}

void TrackView::paint(juce::Graphics &g) {
  // Draw background
  g.fillAll(juce::Colours::darkgrey.darker());

  // Highlight if selected
  if (isSelectedCallback && isSelectedCallback()) {
    g.setColour(juce::Colours::orange.withAlpha(0.3f));
    g.fillRect(getLocalBounds());
  }

  // Draw border
  g.setColour(juce::Colours::grey);
  g.drawRect(getLocalBounds(), 1);

  // Highlight if this track is recording
  if (track.isRecording()) {
    g.setColour(juce::Colours::red.withAlpha(0.3f));
    g.fillRect(getLocalBounds());
  }
}

void TrackView::mouseDown(const juce::MouseEvent &event) {
  if (onTrackClicked)
    onTrackClicked(trackId);
}

void TrackView::resized() {
  auto bounds = getLocalBounds().reduced(5);

  // Layout from top to bottom
  const int buttonHeight = 25;
  const int labelHeight = 20;

  // Track name at top
  trackNameLabel.setBounds(bounds.removeFromTop(labelHeight));
  bounds.removeFromTop(5);

  // Remove button (small, top right)
  auto topRow = bounds.removeFromTop(buttonHeight);
  removeButton.setBounds(topRow.removeFromRight(30));

  // Volume slider (takes most of the space)
  volumeSlider.setBounds(bounds.removeFromTop(120));
  bounds.removeFromTop(10);

  // Record button
  recordButton.setBounds(bounds.removeFromTop(buttonHeight));
  bounds.removeFromTop(5);

  // Play button
  playButton.setBounds(bounds.removeFromTop(buttonHeight));
  bounds.removeFromTop(5);

  // Solo button
  soloButton.setBounds(bounds.removeFromTop(buttonHeight));
  bounds.removeFromTop(5);

  // Clear and Undo buttons
  clearButton.setBounds(bounds.removeFromTop(buttonHeight));
  bounds.removeFromTop(5);
  undoButton.setBounds(bounds.removeFromTop(buttonHeight));
  bounds.removeFromTop(10);

  // Loop count
  loopCountLabel.setBounds(bounds.removeFromTop(labelHeight));
}

void TrackView::updateFromTrack() {
  // Sync UI with track state
  volumeSlider.setValue(track.getVolume(), juce::dontSendNotification);
  soloButton.setToggleState(track.isSoloed(), juce::dontSendNotification);
  recordButton.setToggleState(track.isRecording(), juce::dontSendNotification);
  playButton.setToggleState(track.isPlaying(), juce::dontSendNotification);

  updateButtonStyles();
  refreshLoopCount();
}

void TrackView::refreshLoopCount() {
  int loopCount = static_cast<int>(track.getLooper().getLoops().size());
  loopCountLabel.setText("Loops: " + juce::String(loopCount),
                         juce::dontSendNotification);
}

void TrackView::updateButtonStyles() {
  bool playing = track.isPlaying();
  playButton.setButtonText(playing ? "Stop" : "Play");
  playButton.setToggleState(playing, juce::dontSendNotification);

  bool recording = track.isRecording();
  recordButton.setToggleState(recording, juce::dontSendNotification);
  recordButton.repaint();

  repaint();
}
