#include "GlobalControlBar.h"

GlobalControlBar::GlobalControlBar(juce::AudioProcessorValueTreeState& params)
    : parameters(params)
{
    setupComponents();

    // Add parameter listener
    parameters.addParameterListener("play", this);
    parameters.addParameterListener("monitor", this);
}

GlobalControlBar::~GlobalControlBar()
{
    parameters.removeParameterListener("play", this);
    parameters.removeParameterListener("monitor", this);
}

void GlobalControlBar::setupComponents()
{
    // Title
    titleLabel.setText("Multi-Track Looper", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::left);
    addAndMakeVisible(titleLabel);

    // Play button (attached to parameter)
    playButton.setButtonText("Play");
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
    playButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::green);
    playButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    playButton.setClickingTogglesState(true);
    playAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(parameters, "play", playButton);
    playButton.onClick = [this]() {
        if (onPlayChanged)
            onPlayChanged(playButton.getToggleState());
    };
    addAndMakeVisible(playButton);

    // Monitor button (attached to parameter)
    monitorButton.setButtonText("Monitor");
    monitorButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
    monitorButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::blue);
    monitorButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    monitorButton.setClickingTogglesState(true);
    monitorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(parameters, "monitor", monitorButton);
    monitorButton.onClick = [this]() {
        if (onMonitorChanged)
            onMonitorChanged(monitorButton.getToggleState());
    };
    addAndMakeVisible(monitorButton);

    // Clear All button
    clearAllButton.setButtonText("Clear All");
    clearAllButton.onClick = [this]() {
        if (onClearAll)
        {
            onClearAll();
        }
    };
    addAndMakeVisible(clearAllButton);

    // Undo Last button
    undoLastButton.setButtonText("Undo Last");
    undoLastButton.onClick = [this]() {
        if (onUndoLast)
        {
            onUndoLast();
        }
    };
    addAndMakeVisible(undoLastButton);

    // Status label
    statusLabel.setText("Ready", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::right);
    addAndMakeVisible(statusLabel);

    // Info label
    infoLabel.setText("Tracks: 0 | Loops: 0", juce::dontSendNotification);
    infoLabel.setJustificationType(juce::Justification::right);
    addAndMakeVisible(infoLabel);

    updateButtonStyles();
}

void GlobalControlBar::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colours::darkgrey);

    // Bottom border
    g.setColour(juce::Colours::grey);
    g.drawHorizontalLine(getHeight() - 1, 0, static_cast<float>(getWidth()));
}

void GlobalControlBar::resized()
{
    auto bounds = getLocalBounds().reduced(5);

    const int buttonWidth = 80;
    const int buttonHeight = 30;
    const int smallButtonWidth = 70;
    const int labelHeight = 25;

    // Top row: Title on left, status on right
    auto topRow = bounds.removeFromTop(labelHeight);
    titleLabel.setBounds(topRow.removeFromLeft(200));
    statusLabel.setBounds(topRow);
    bounds.removeFromTop(5);

    // Bottom row: Buttons and info
    auto bottomRow = bounds.removeFromTop(buttonHeight);

    // Play button
    playButton.setBounds(bottomRow.removeFromLeft(buttonWidth));
    bottomRow.removeFromLeft(10);

    // Monitor button
    monitorButton.setBounds(bottomRow.removeFromLeft(buttonWidth));
    bottomRow.removeFromLeft(20);

    // Clear and Undo buttons
    clearAllButton.setBounds(bottomRow.removeFromLeft(smallButtonWidth));
    bottomRow.removeFromLeft(5);
    undoLastButton.setBounds(bottomRow.removeFromLeft(smallButtonWidth));

    // Info label on the right
    infoLabel.setBounds(bottomRow.removeFromRight(150));
}

void GlobalControlBar::parameterChanged(const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused(newValue);

    if (parameterID == "play")
    {
        updateButtonStyles();
    }
    else if (parameterID == "monitor")
    {
        updateButtonStyles();
    }
}

void GlobalControlBar::updateButtonStyles()
{
    // Update status text based on play state
    // Button colors are handled automatically by JUCE via buttonOnColourId
    if (playButton.getToggleState())
        setStatusText("Playing");
    else
        setStatusText("Stopped");
}

void GlobalControlBar::setStatusText(const juce::String& text)
{
    statusLabel.setText(text, juce::dontSendNotification);
}

void GlobalControlBar::setLoopInfo(int trackCount, int totalLoops)
{
    infoLabel.setText("Tracks: " + juce::String(trackCount) + " | Loops: " + juce::String(totalLoops),
                      juce::dontSendNotification);
}
