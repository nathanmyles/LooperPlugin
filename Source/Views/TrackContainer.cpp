#include "TrackContainer.h"

TrackContainer::TrackContainer()
    : trackList(*this)
{
    setupComponents();
}

TrackContainer::~TrackContainer()
{
}

void TrackContainer::setupComponents()
{
    // Setup viewport
    viewport.setViewedComponent(&trackList, false);
    viewport.setScrollBarsShown(true, true);
    addAndMakeVisible(viewport);

    // Setup add track button
    addTrackButton.setButtonText("+ Add Track");
    addTrackButton.onClick = [this]() {
        if (onAddTrack)
        {
            onAddTrack();
        }
    };
    addAndMakeVisible(addTrackButton);
}

void TrackContainer::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
}

void TrackContainer::resized()
{
    auto bounds = getLocalBounds();

    // Add track button at the bottom
    const int buttonHeight = 30;
    addTrackButton.setBounds(bounds.removeFromBottom(buttonHeight).reduced(5));
    bounds.removeFromBottom(5);

    // Viewport takes the rest
    viewport.setBounds(bounds);

    // Update track list size
    int totalWidth = static_cast<int>(trackList.getTrackViews().size()) * trackWidth;
    trackList.setSize(juce::jmax(totalWidth, bounds.getWidth()), bounds.getHeight());
}

void TrackContainer::addTrackView(Track* track)
{
    if (track == nullptr)
        return;

    // Create track view - TrackView stores reference to track
    auto trackView = std::make_unique<TrackView>(track->getId(), *track);

    // Setup callbacks
    trackView->onRemoveTrack = [this](int trackId) {
        if (onRemoveTrack)
        {
            onRemoveTrack(trackId);
        }
    };

    trackView->onRecordClicked = [this](int trackId, bool isRecording) {
        if (onRecordTrack)
        {
            onRecordTrack(trackId, isRecording);
        }
    };

    trackView->onClearTrack = [this](int trackId) {
        if (onClearTrack)
        {
            onClearTrack(trackId);
        }
    };

    trackView->onUndoTrack = [this](int trackId) {
        if (onUndoTrack)
        {
            onUndoTrack(trackId);
        }
    };

    trackList.addTrackView(std::move(trackView));

    // Update layout
    resized();
}

void TrackContainer::removeTrackView(int trackId)
{
    // Remove from track list (UI only)
    trackList.removeTrackView(trackId);

    // Update layout
    resized();
}

void TrackContainer::removeAllTrackViews()
{
    trackList.removeAllTrackViews();
    resized();
}

void TrackContainer::refreshTrackViews()
{
    for (auto& trackView : trackList.getTrackViews())
    {
        trackView->refreshLoopCount();
    }
}

// TrackListComponent implementation

TrackContainer::TrackListComponent::TrackListComponent(TrackContainer& owner)
    : owner(owner)
{
}

void TrackContainer::TrackListComponent::resized()
{
    auto bounds = getLocalBounds();

    // Layout track views horizontally
    for (auto& trackView : trackViews)
    {
        trackView->setBounds(bounds.removeFromLeft(owner.trackWidth));
    }
}

void TrackContainer::TrackListComponent::addTrackView(std::unique_ptr<TrackView> trackView)
{
    trackViews.push_back(std::move(trackView));
    addAndMakeVisible(trackViews.back().get());
    resized();
}

void TrackContainer::TrackListComponent::removeTrackView(int trackId)
{
    auto it = std::find_if(trackViews.begin(), trackViews.end(),
        [trackId](const std::unique_ptr<TrackView>& tv) { return tv->getTrackId() == trackId; });

    if (it != trackViews.end())
    {
        trackViews.erase(it);
        resized();
    }
}

void TrackContainer::TrackListComponent::removeAllTrackViews()
{
    trackViews.clear();
    resized();
}

TrackView* TrackContainer::TrackListComponent::findTrackView(int trackId)
{
    auto it = std::find_if(trackViews.begin(), trackViews.end(),
        [trackId](const std::unique_ptr<TrackView>& tv) { return tv->getTrackId() == trackId; });

    return (it != trackViews.end()) ? it->get() : nullptr;
}
