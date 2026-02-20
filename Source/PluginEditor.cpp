#include "PluginProcessor.h"
#include "PluginEditor.h"

LooperAudioProcessorEditor::LooperAudioProcessorEditor(LooperAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      controlBar(audioProcessor.parameters),
      trackContainer()
{
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
}

LooperAudioProcessorEditor::~LooperAudioProcessorEditor()
{
    stopTimer();
}

void LooperAudioProcessorEditor::setupCallbacks()
{
    // Control bar callbacks
    controlBar.onPlayChanged = [this](bool isPlaying) {
        if (isPlaying)
        {
            audioProcessor.startPlayback();
        }
        else
        {
            audioProcessor.stopPlayback();
        }
    };

    controlBar.onMonitorChanged = [this](bool isMonitoring) {
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

    controlBar.onUndoLast = [this]() {
        audioProcessor.requestUndoLast();
    };

    // Track container callbacks
    trackContainer.onAddTrack = [this]() {
        // Add track to processor
        Track* newTrack = audioProcessor.addTrack();
        if (newTrack != nullptr)
        {
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

    trackContainer.onRecordTrack = [this](int trackId, bool isRecording) {
        if (isRecording)
        {
            audioProcessor.startRecordingTrack(trackId);
        }
        else
        {
            audioProcessor.stopRecordingTrack(trackId);
        }
        updateTrackButtons();
    };

    trackContainer.onClearTrack = [this](int trackId) {
        audioProcessor.clearTrack(trackId);
    };

    trackContainer.onUndoTrack = [this](int trackId) {
        audioProcessor.undoTrack(trackId);
    };
}

void LooperAudioProcessorEditor::addInitialTrack()
{
    // If no tracks exist in the processor, add one
    if (audioProcessor.getTrackCount() == 0)
    {
        Track* track = audioProcessor.addTrack();
        if (track != nullptr)
        {
            trackContainer.addTrackView(track);
        }
    }
    else
    {
        // Tracks were restored from state - add them to UI
        syncTracksWithProcessor();
    }

    updateTrackButtons();
}

void LooperAudioProcessorEditor::syncTracksWithProcessor()
{
    // Clear existing UI track views
    trackContainer.removeAllTrackViews();

    // Add track views for all tracks in processor
    for (auto& track : audioProcessor.getTracks())
    {
        trackContainer.addTrackView(track.get());
    }
}

void LooperAudioProcessorEditor::updateTrackButtons()
{
    // Enable/disable record buttons based on recording state
    // Only one track can record at a time
    bool anyRecording = false;
    for (auto& track : audioProcessor.getTracks())
    {
        if (track->isRecording())
        {
            anyRecording = true;
            break;
        }
    }

    // Update control bar info
    int totalLoops = 0;
    for (auto& track : audioProcessor.getTracks())
    {
        totalLoops += static_cast<int>(track->getLooper().getLoops().size());
    }
    controlBar.setLoopInfo(audioProcessor.getTrackCount(), totalLoops);
}

void LooperAudioProcessorEditor::timerCallback()
{
    // Update loop counts in track views
    trackContainer.refreshTrackViews();

    // Update track info display
    updateTrackButtons();
}

void LooperAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
}

void LooperAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Control bar at the top (80px height)
    controlBar.setBounds(bounds.removeFromTop(80));

    // Track container takes the rest
    trackContainer.setBounds(bounds);
}
