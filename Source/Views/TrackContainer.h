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
#include "TrackView.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

/**
 * TrackContainer - Holds all track views in a horizontally scrollable area
 *
 * Layout:
 * - Viewport with horizontal scrolling
 * - Track views arranged horizontally
 * - Add Track button at the right end
 */
class TrackContainer : public juce::Component {
public:
  // Callbacks
  std::function<void()> onAddTrack;
  std::function<void(int)> onRemoveTrack;
  std::function<void(int, bool)> onRecordTrack;
  std::function<void(int, bool)> onPlayTrack;
  std::function<void(int)> onClearTrack;
  std::function<void(int)> onUndoTrack;
  std::function<void(int)> onSelectedTrackChanged;
  std::function<void()> onRefreshUI;

  TrackContainer();
  ~TrackContainer() override;

  // Track management - TrackContainer does NOT own tracks, just the views
  void addTrackView(
      Track *track); // Takes raw pointer - track owned by PluginProcessor
  void removeTrackView(int trackId);
  void removeAllTrackViews();

  // Refresh all track views (call during timer callback)
  void refreshTrackViews();

  // Selection
  void selectTrack(int trackId);
  int getSelectedTrackId() const { return selectedTrackId; }

  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  // Viewport for scrolling
  juce::Viewport viewport;

  // Container that holds the actual track views
  class TrackListComponent : public juce::Component {
  public:
    TrackListComponent(TrackContainer &owner);
    void resized() override;
    void addTrackView(std::unique_ptr<TrackView> trackView);
    void removeTrackView(int trackId);
    void removeAllTrackViews();
    TrackView *findTrackView(int trackId);
    std::vector<std::unique_ptr<TrackView>> &getTrackViews() {
      return trackViews;
    }

  private:
    TrackContainer &owner;
    std::vector<std::unique_ptr<TrackView>> trackViews;
  };

  TrackListComponent trackList;

  int selectedTrackId = -1;

  // Add track button
  juce::TextButton addTrackButton;

  // Constants
  static constexpr int trackWidth = 120;
  static constexpr int trackHeight = 350;

  void setupComponents();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackContainer)
};
