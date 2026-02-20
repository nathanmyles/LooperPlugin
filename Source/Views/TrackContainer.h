#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Models/Track.h"
#include "TrackView.h"

/**
 * TrackContainer - Holds all track views in a horizontally scrollable area
 *
 * Layout:
 * - Viewport with horizontal scrolling
 * - Track views arranged horizontally
 * - Add Track button at the right end
 */
class TrackContainer : public juce::Component
{
public:
    // Callbacks
    std::function<void()> onAddTrack;
    std::function<void(int)> onRemoveTrack;
    std::function<void(int, bool)> onRecordTrack;
    std::function<void(int)> onClearTrack;
    std::function<void(int)> onUndoTrack;
    std::function<void()> onRefreshUI;

    TrackContainer();
    ~TrackContainer() override;

    // Track management - TrackContainer does NOT own tracks, just the views
    void addTrackView(Track* track);  // Takes raw pointer - track owned by PluginProcessor
    void removeTrackView(int trackId);
    void removeAllTrackViews();

    // Refresh all track views (call during timer callback)
    void refreshTrackViews();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    // Viewport for scrolling
    juce::Viewport viewport;

    // Container that holds the actual track views
    class TrackListComponent : public juce::Component
    {
    public:
        TrackListComponent(TrackContainer& owner);
        void resized() override;
        void addTrackView(std::unique_ptr<TrackView> trackView);
        void removeTrackView(int trackId);
        void removeAllTrackViews();
        TrackView* findTrackView(int trackId);
        std::vector<std::unique_ptr<TrackView>>& getTrackViews() { return trackViews; }

    private:
        TrackContainer& owner;
        std::vector<std::unique_ptr<TrackView>> trackViews;
    };

    TrackListComponent trackList;

    // Add track button
    juce::TextButton addTrackButton;

    // Constants
    static constexpr int trackWidth = 120;
    static constexpr int trackHeight = 350;

    void setupComponents();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackContainer)
};
