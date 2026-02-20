#include "TrackView.h"

TrackView::TrackView(int id, Track& t)
    : trackId(id), track(t)
{
    setupComponents();
    updateFromTrack();
}

TrackView::~TrackView()
{
}

void TrackView::setupComponents()
{
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
    recordButton.setButtonText("REC");
    recordButton.setColour(juce::TextButton::buttonColourId, juce::Colours::grey);
    recordButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);
    recordButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    recordButton.setClickingTogglesState(true);
    recordButton.onClick = [this]() {
        if (onRecordClicked)
            onRecordClicked(trackId, recordButton.getToggleState());
    };
    addAndMakeVisible(recordButton);

    // Mute button
    muteButton.setButtonText("M");
    muteButton.setColour(juce::TextButton::buttonColourId, juce::Colours::grey);
    muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
    muteButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    muteButton.setClickingTogglesState(true);
    muteButton.onClick = [this]() {
        track.setMuted(muteButton.getToggleState());
    };
    addAndMakeVisible(muteButton);

    // Solo button
    soloButton.setButtonText("S");
    soloButton.setColour(juce::TextButton::buttonColourId, juce::Colours::grey);
    soloButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::yellow);
    soloButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    soloButton.setClickingTogglesState(true);
    soloButton.onClick = [this]() {
        track.setSoloed(soloButton.getToggleState());
    };
    addAndMakeVisible(soloButton);

    // Clear button
    clearButton.setButtonText("Clear");
    clearButton.onClick = [this]() {
        if (onClearTrack)
        {
            onClearTrack(trackId);
        }
    };
    addAndMakeVisible(clearButton);

    // Undo button
    undoButton.setButtonText("Undo");
    undoButton.onClick = [this]() {
        if (onUndoTrack)
        {
            onUndoTrack(trackId);
        }
    };
    addAndMakeVisible(undoButton);

    // Remove button
    removeButton.setButtonText("X");
    removeButton.onClick = [this]() {
        if (onRemoveTrack)
        {
            onRemoveTrack(trackId);
        }
    };
    addAndMakeVisible(removeButton);

    // Loop count label
    loopCountLabel.setText("Loops: 0", juce::dontSendNotification);
    loopCountLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(loopCountLabel);

    // Status label
    statusLabel.setText("Ready", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusLabel);

    updateButtonStyles();
}

void TrackView::paint(juce::Graphics& g)
{
    // Draw background
    g.fillAll(juce::Colours::darkgrey.darker());

    // Draw border
    g.setColour(juce::Colours::grey);
    g.drawRect(getLocalBounds(), 1);

    // Highlight if this track is recording
    if (track.isRecording())
    {
        g.setColour(juce::Colours::red.withAlpha(0.3f));
        g.fillRect(getLocalBounds());
    }
}

void TrackView::resized()
{
    auto bounds = getLocalBounds().reduced(5);

    // Layout from top to bottom
    const int buttonHeight = 25;
    const int smallButtonWidth = 30;
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

    // Mute and Solo buttons side by side
    auto muteSoloRow = bounds.removeFromTop(buttonHeight);
    muteButton.setBounds(muteSoloRow.removeFromLeft(muteSoloRow.getWidth() / 2 - 2));
    soloButton.setBounds(muteSoloRow.removeFromRight(muteSoloRow.getWidth()));
    bounds.removeFromTop(5);

    // Clear and Undo buttons
    clearButton.setBounds(bounds.removeFromTop(buttonHeight));
    bounds.removeFromTop(5);
    undoButton.setBounds(bounds.removeFromTop(buttonHeight));
    bounds.removeFromTop(10);

    // Loop count
    loopCountLabel.setBounds(bounds.removeFromTop(labelHeight));
    bounds.removeFromTop(5);

    // Status label at bottom
    statusLabel.setBounds(bounds.removeFromTop(labelHeight));
}

void TrackView::updateFromTrack()
{
    // Sync UI with track state
    volumeSlider.setValue(track.getVolume(), juce::dontSendNotification);
    muteButton.setToggleState(track.isMuted(), juce::dontSendNotification);
    soloButton.setToggleState(track.isSoloed(), juce::dontSendNotification);

    updateButtonStyles();
    refreshLoopCount();
}

void TrackView::refreshLoopCount()
{
    int loopCount = static_cast<int>(track.getLooper().getLoops().size());
    loopCountLabel.setText("Loops: " + juce::String(loopCount), juce::dontSendNotification);
}

void TrackView::updateButtonStyles()
{
    // Button colors are handled automatically by JUCE via buttonOnColourId
    // Just update the status label
    updateStatusLabel();
}

void TrackView::updateStatusLabel()
{
    if (track.isRecording())
    {
        statusLabel.setText("Recording...", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    }
    else if (track.isPlaying())
    {
        statusLabel.setText("Playing", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::green);
    }
    else
    {
        statusLabel.setText("Ready", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    }
}
