#include "LooperView.h"

LooperView::LooperView(juce::AudioProcessorValueTreeState& params,
                       const std::vector<std::unique_ptr<Looper::Loop>>& loopsRef)
    : parameters(params), loops(loopsRef)
{
    setupComponents();

    // Add parameter listeners
    parameters.addParameterListener("record", this);
    parameters.addParameterListener("play", this);
    parameters.addParameterListener("monitor", this);

    // Trigger initial update
    parameterChanged("record", *parameters.getRawParameterValue("record"));
    parameterChanged("play", *parameters.getRawParameterValue("play"));
    parameterChanged("monitor", *parameters.getRawParameterValue("monitor"));

    // Set initial loop count
    updateLoopCount();
}

LooperView::~LooperView()
{
    parameters.removeParameterListener("record", this);
    parameters.removeParameterListener("play", this);
    parameters.removeParameterListener("monitor", this);
}

void LooperView::setupComponents()
{
    // Title
    titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    titleLabel.setText("Looper", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    // Volume slider
    volumeSlider.setSliderStyle(juce::Slider::LinearVertical);
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    volumeSlider.setRange(0.0, 1.0, 0.1);
    volumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        parameters, "volume", volumeSlider);
    addAndMakeVisible(volumeSlider);

    volumeLabel.setFont(juce::FontOptions(12.0f));
    volumeLabel.setText("Volume", juce::dontSendNotification);
    volumeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(volumeLabel);

    // Monitor button
    monitorButton.setButtonText("Monitor: Off");
    monitorButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
    monitorButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::royalblue);
    monitorButton.setClickingTogglesState(true);
    monitorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        parameters, "monitor", monitorButton);
    addAndMakeVisible(monitorButton);

    // Record button
    recordButton.setButtonText("Record");
    recordButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
    recordButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);
    recordButton.setClickingTogglesState(true);
    recordAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        parameters, "record", recordButton);
    addAndMakeVisible(recordButton);

    // Play button
    playButton.setButtonText("Play");
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgreen);
    playButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::green);
    playButton.setClickingTogglesState(true);
    playAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        parameters, "play", playButton);
    addAndMakeVisible(playButton);

    // Clear button
    clearButton.setButtonText("Undo All");
    clearButton.setColour(juce::TextButton::buttonColourId, juce::Colours::grey);
    clearButton.onClick = [this] {
        if (onClearAll)
            onClearAll();
        // Wait a bit for audio thread to process, then update UI
        juce::Timer::callAfterDelay(50, [this] { updateLoopCount(); });
    };
    addAndMakeVisible(clearButton);

    // Undo button
    undoButton.setButtonText("Undo");
    undoButton.setColour(juce::TextButton::buttonColourId, juce::Colours::orange);
    undoButton.onClick = [this] {
        if (onUndoLast)
            onUndoLast();
        // Wait a bit for audio thread to process, then update UI
        juce::Timer::callAfterDelay(50, [this] { updateLoopCount(); });
    };
    addAndMakeVisible(undoButton);

    // Status label
    statusLabel.setFont(juce::FontOptions(12.0f));
    statusLabel.setText("Ready", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusLabel);

    // Loop count label
    loopCountLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    loopCountLabel.setText("Loops: 0", juce::dontSendNotification);
    loopCountLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(loopCountLabel);
}

void LooperView::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
}

void LooperView::resized()
{
    auto bounds = getLocalBounds();

    // Title at the top (y: 0-30)
    titleLabel.setBounds(0, 0, bounds.getWidth(), 30);

    // Volume label (y: 40-60)
    volumeLabel.setBounds(0, 40, bounds.getWidth(), 20);

    // Volume slider (y: 60-220)
    auto sliderWidth = 60;
    auto sliderHeight = 160;
    auto sliderX = (bounds.getWidth() - sliderWidth) / 2;
    volumeSlider.setBounds(sliderX, 60, sliderWidth, sliderHeight);

    // Buttons
    int buttonWidth = juce::jmin(140, bounds.getWidth() - 20);
    int buttonHeight = 30;
    int buttonSpacing = 10;
    int buttonX = (bounds.getWidth() - buttonWidth) / 2;

    monitorButton.setBounds(buttonX, 230, buttonWidth, buttonHeight);
    recordButton.setBounds(buttonX, 230 + buttonHeight + buttonSpacing, buttonWidth, buttonHeight);
    playButton.setBounds(buttonX, 230 + (buttonHeight + buttonSpacing) * 2, buttonWidth, buttonHeight);
    clearButton.setBounds(buttonX, 230 + (buttonHeight + buttonSpacing) * 3, buttonWidth, buttonHeight);
    undoButton.setBounds(buttonX, 230 + (buttonHeight + buttonSpacing) * 4, buttonWidth, buttonHeight);

    // Status label (below buttons)
    statusLabel.setBounds(0, 430, bounds.getWidth(), 30);

    // Loop count label (at bottom)
    loopCountLabel.setBounds(0, 465, bounds.getWidth(), 25);
}

void LooperView::updateLoopCount()
{
    int loopCount = static_cast<int>(loops.size());
    loopCountLabel.setText("Loops: " + juce::String(loopCount), juce::dontSendNotification);
}

void LooperView::updateStatusLabel()
{
    bool isRecording = recordButton.getToggleState();
    bool isPlaying = playButton.getToggleState();

    if (isRecording)
    {
        statusLabel.setText("Recording...", juce::dontSendNotification);
    }
    else if (isPlaying)
    {
        statusLabel.setText("Playing...", juce::dontSendNotification);
    }
    else
    {
        statusLabel.setText("Ready", juce::dontSendNotification);
    }
}

void LooperView::parameterChanged(const juce::String& parameterID, float newValue)
{
    juce::MessageManager::callAsync([this, parameterID, newValue] {
        // Update loop count display
        updateLoopCount();

        if (parameterID == "monitor")
        {
            bool isMonitoring = newValue > 0.5f;
            if (isMonitoring)
            {
                monitorButton.setButtonText("Monitor: On");
            }
            else
            {
                monitorButton.setButtonText("Monitor: Off");
            }
        }
        else if (parameterID == "record")
        {
            bool isRecording = newValue > 0.5f;
            if (isRecording)
            {
                recordButton.setButtonText("Stop");
            }
            else
            {
                recordButton.setButtonText("Record");
            }
            updateStatusLabel();
        }
        else if (parameterID == "play")
        {
            bool isPlaying = newValue > 0.5f;
            if (isPlaying)
            {
                playButton.setButtonText("Stop");
            }
            else
            {
                playButton.setButtonText("Play");
            }
            updateStatusLabel();
        }
    });
}
