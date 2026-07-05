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

#include "TrackContainer.h"

TrackContainer::TrackContainer() : trackList(*this) { setupComponents(); }

TrackContainer::~TrackContainer() {}

void TrackContainer::setupComponents() {
  // Setup viewport
  viewport.setViewedComponent(&trackList, false);
  viewport.setScrollBarsShown(true, true);
  addAndMakeVisible(viewport);

  // Setup add track button
  addTrackButton.setButtonText("+ Add Track");
  addTrackButton.onClick = [this]() {
    if (onAddTrack) {
      onAddTrack();
    }
  };
  addAndMakeVisible(addTrackButton);
}

void TrackContainer::paint(juce::Graphics &g) {
  g.fillAll(juce::Colours::darkgrey);
}

void TrackContainer::resized() {
  auto bounds = getLocalBounds();

  // Add track button at the bottom
  const int buttonHeight = 30;
  addTrackButton.setBounds(bounds.removeFromBottom(buttonHeight).reduced(5));
  bounds.removeFromBottom(5);

  // Viewport takes the rest
  viewport.setBounds(bounds);

  // Update track list size
  int totalWidth =
      static_cast<int>(trackList.getTrackViews().size()) * trackWidth;
  trackList.setSize(juce::jmax(totalWidth, bounds.getWidth()),
                    bounds.getHeight());
}

void TrackContainer::addTrackView(Track *track) {
  if (track == nullptr)
    return;

  // Create track view - TrackView stores reference to track
  auto trackView = std::make_unique<TrackView>(track->getId(), *track);

  // Setup callbacks
  trackView->onRemoveTrack = [this](int trackId) {
    if (onRemoveTrack) {
      onRemoveTrack(trackId);
    }
  };

  trackView->onRecordClicked = [this](int trackId, bool isRecording) {
    if (onRecordTrack) {
      onRecordTrack(trackId, isRecording);
    }
  };

  trackView->onPlayClicked = [this](int trackId, bool isPlaying) {
    if (onPlayTrack) {
      onPlayTrack(trackId, isPlaying);
    }
  };

  trackView->onClearTrack = [this](int trackId) {
    if (onClearTrack) {
      onClearTrack(trackId);
    }
  };

  trackView->onUndoTrack = [this](int trackId) {
    if (onUndoTrack) {
      onUndoTrack(trackId);
    }
  };

  trackView->onTrackClicked = [this](int trackId) { selectTrack(trackId); };

  int id = track->getId();
  trackView->isSelectedCallback = [this, id]() {
    return id == selectedTrackId;
  };

  trackList.addTrackView(std::move(trackView));

  // Update layout
  resized();
}

void TrackContainer::removeTrackView(int trackId) {
  if (selectedTrackId == trackId)
    selectedTrackId = -1;

  trackList.removeTrackView(trackId);

  resized();
}

void TrackContainer::removeAllTrackViews() {
  trackList.removeAllTrackViews();
  resized();
}

void TrackContainer::refreshTrackViews() {
  for (auto &trackView : trackList.getTrackViews()) {
    trackView->updateFromTrack();
  }
}

void TrackContainer::selectTrack(int trackId) {
  if (selectedTrackId == trackId)
    return;

  int oldSelected = selectedTrackId;
  selectedTrackId = trackId;

  auto &views = trackList.getTrackViews();
  for (auto &view : views) {
    int id = view->getTrackId();
    if (id == oldSelected || id == selectedTrackId) {
      view->repaint();
    }
  }

  if (onSelectedTrackChanged) {
    onSelectedTrackChanged(trackId);
  }
}

// TrackListComponent implementation

TrackContainer::TrackListComponent::TrackListComponent(TrackContainer &owner)
    : owner(owner) {}

void TrackContainer::TrackListComponent::resized() {
  auto bounds = getLocalBounds();

  // Layout track views horizontally
  for (auto &trackView : trackViews) {
    trackView->setBounds(bounds.removeFromLeft(owner.trackWidth));
  }
}

void TrackContainer::TrackListComponent::addTrackView(
    std::unique_ptr<TrackView> trackView) {
  trackViews.push_back(std::move(trackView));
  addAndMakeVisible(trackViews.back().get());
  resized();
}

void TrackContainer::TrackListComponent::removeTrackView(int trackId) {
  auto it = std::find_if(trackViews.begin(), trackViews.end(),
                         [trackId](const std::unique_ptr<TrackView> &tv) {
                           return tv->getTrackId() == trackId;
                         });

  if (it != trackViews.end()) {
    trackViews.erase(it);
    resized();
  }
}

void TrackContainer::TrackListComponent::removeAllTrackViews() {
  trackViews.clear();
  resized();
}

TrackView *TrackContainer::TrackListComponent::findTrackView(int trackId) {
  auto it = std::find_if(trackViews.begin(), trackViews.end(),
                         [trackId](const std::unique_ptr<TrackView> &tv) {
                           return tv->getTrackId() == trackId;
                         });

  return (it != trackViews.end()) ? it->get() : nullptr;
}
