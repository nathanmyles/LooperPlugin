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

#include "PluginEditor.h"
#include "PluginProcessor.h"

LooperAudioProcessorEditor::LooperAudioProcessorEditor(LooperAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      controlBar(audioProcessor.parameters), trackContainer() {
  // Set initial size and limits
  setSize(800, 500);
  setResizeLimits(600, 350, 1200, 600);

  // Setup callbacks
  setupCallbacks();

  // Add control bar and track container
  addAndMakeVisible(controlBar);
  addAndMakeVisible(trackContainer);

  // Add initial track if none exist
  addInitialTrack();

  // Start timer for UI updates (30fps)
  startTimer(33);

  // Intercept key events across all child components
  addKeyListener(this);
}

LooperAudioProcessorEditor::~LooperAudioProcessorEditor() { stopTimer(); }

void LooperAudioProcessorEditor::setupCallbacks() {
  // Control bar callbacks
  controlBar.onPlayChanged = [this](bool isPlaying) {
    if (isPlaying) {
      audioProcessor.startPlayback();
    } else {
      audioProcessor.stopPlayback();
    }
  };

  controlBar.onStopAll = [this]() {
    audioProcessor.stopPlayback();
    auto *playParam = audioProcessor.parameters.getParameter("playAll");
    if (playParam)
      playParam->setValueNotifyingHost(0.0f);
  };

  controlBar.onMonitorChanged = [](bool isMonitoring) {
    // Monitor state is handled automatically by the processor
    juce::ignoreUnused(isMonitoring);
  };

  controlBar.onClearAll = [this]() {
    audioProcessor.requestClearAll();
    // Clear all UI track views after clearing tracks
    trackContainer.removeAllTrackViews();
    // Re-add any remaining tracks (if tracks are cleared but not removed)
    syncTracksWithProcessor();
  };

  controlBar.onUndoLast = [this]() { audioProcessor.requestUndoLast(); };

  // Track container callbacks
  trackContainer.onAddTrack = [this]() {
    // Add track to processor
    Track *newTrack = audioProcessor.addTrack();
    if (newTrack != nullptr) {
      // Add corresponding track view to UI
      trackContainer.addTrackView(newTrack);
    }
    updateTrackButtons();
  };

  trackContainer.onRemoveTrack = [this](int trackId) {
    // Remove track from processor
    audioProcessor.removeTrack(trackId);
    // Remove corresponding track view from UI
    trackContainer.removeTrackView(trackId);
    updateTrackButtons();
  };

  trackContainer.onSelectedTrackChanged = [this](int trackId) {
    audioProcessor.setCurrentTrackId(trackId);
  };

  trackContainer.onRecordTrack = [this](int trackId, bool isRecording) {
    if (isRecording) {
      audioProcessor.startRecordingTrack(trackId);
    } else {
      audioProcessor.stopRecordingTrack(trackId);
    }
    if (trackId == audioProcessor.getCurrentTrackId()) {
      audioProcessor.syncParamsWithCurrentTrack();
    }
    trackContainer.refreshTrackViews();
    updateTrackButtons();
  };

  trackContainer.onPlayTrack = [this](int trackId, bool isPlaying) {
    if (isPlaying) {
      audioProcessor.startPlaybackTrack(trackId);
    } else {
      if (audioProcessor.findTrack(trackId) != nullptr &&
          audioProcessor.findTrack(trackId)->isRecording()) {
        audioProcessor.stopRecordingTrack(trackId);
      } else {
        audioProcessor.stopPlaybackTrack(trackId);
      }
    }
    if (trackId == audioProcessor.getCurrentTrackId()) {
      audioProcessor.syncParamsWithCurrentTrack();
    }
    trackContainer.refreshTrackViews();
    updateTrackButtons();
  };

  trackContainer.onClearTrack = [this](int trackId) {
    audioProcessor.clearTrack(trackId);
  };

  trackContainer.onUndoTrack = [this](int trackId) {
    audioProcessor.undoTrack(trackId);
  };
}

void LooperAudioProcessorEditor::addInitialTrack() {
  // If no tracks exist in the processor, add one
  if (audioProcessor.getTrackCount() == 0) {
    Track *track = audioProcessor.addTrack();
    if (track != nullptr) {
      trackContainer.addTrackView(track);
    }
  } else {
    // Tracks were restored from state - add them to UI
    syncTracksWithProcessor();
  }

  // Select the first track by default
  if (audioProcessor.getTrackCount() > 0) {
    auto tracks = audioProcessor.getTracks();
    if (!tracks.empty()) {
      trackContainer.selectTrack(tracks[0]->getId());
    }
  }

  updateTrackButtons();
}

void LooperAudioProcessorEditor::syncTracksWithProcessor() {
  // Clear existing UI track views
  trackContainer.removeAllTrackViews();

  // Add track views for all tracks in processor
  for (auto *track : audioProcessor.getTracks()) {
    trackContainer.addTrackView(track);
  }
}

void LooperAudioProcessorEditor::updateTrackButtons() {
  bool anyPlaying = false;
  auto tracks = audioProcessor.getTracks();
  for (auto *track : tracks) {
    if (track->isPlaying())
      anyPlaying = true;
  }

  controlBar.setPlayAllButtonState(anyPlaying);

  // Update control bar info
  int totalLoops = 0;
  for (auto *track : tracks) {
    totalLoops += static_cast<int>(track->getLooper().getNumLoops());
  }
  controlBar.setLoopInfo(audioProcessor.getTrackCount(), totalLoops);
}

bool LooperAudioProcessorEditor::keyPressed(const juce::KeyPress &key,
                                            juce::Component *) {
  // Don't consume typed characters when editing text
  if (dynamic_cast<juce::TextEditor *>(
          juce::Component::getCurrentlyFocusedComponent()) != nullptr) {
    return false;
  }

  int selectedId = trackContainer.getSelectedTrackId();
  if (selectedId < 0)
    return false;

  Track *track = audioProcessor.findTrack(selectedId);
  if (track == nullptr)
    return false;

  if (key == juce::KeyPress('r') || key == juce::KeyPress('R')) {
    if (track->isRecording()) {
      audioProcessor.stopRecordingTrack(selectedId);
    } else {
      audioProcessor.startRecordingTrack(selectedId);
    }
    if (selectedId == audioProcessor.getCurrentTrackId()) {
      audioProcessor.syncParamsWithCurrentTrack();
    }
    trackContainer.refreshTrackViews();
    updateTrackButtons();
    return true;
  }

  if (key == juce::KeyPress(' ')) {
    if (track->isPlaying()) {
      audioProcessor.stopPlaybackTrack(selectedId);
    } else {
      audioProcessor.startPlaybackTrack(selectedId);
    }
    if (selectedId == audioProcessor.getCurrentTrackId())
      audioProcessor.syncParamsWithCurrentTrack();
    trackContainer.refreshTrackViews();
    updateTrackButtons();
    return true;
  }

  if (key == juce::KeyPress::backspaceKey) {
    audioProcessor.clearTrack(selectedId);
    audioProcessor.syncParamsWithCurrentTrack();
    trackContainer.refreshTrackViews();
    return true;
  }

  if (key == juce::KeyPress('z') && key.getModifiers().isCtrlDown()) {
    audioProcessor.undoTrack(selectedId);
    audioProcessor.syncParamsWithCurrentTrack();
    trackContainer.refreshTrackViews();
    return true;
  }

  return false;
}

void LooperAudioProcessorEditor::timerCallback() {
  // Update loop counts in track views
  trackContainer.refreshTrackViews();

  // Update track info display
  updateTrackButtons();
}

void LooperAudioProcessorEditor::paint(juce::Graphics &g) {
  g.fillAll(juce::Colours::darkgrey);
}

void LooperAudioProcessorEditor::resized() {
  auto bounds = getLocalBounds();

  // Control bar at the top (80px height)
  controlBar.setBounds(bounds.removeFromTop(80));

  // Track container takes the rest
  trackContainer.setBounds(bounds);
}
