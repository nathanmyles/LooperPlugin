#include "PluginProcessor.h"
#include "PluginEditor.h"

LooperAudioProcessorEditor::LooperAudioProcessorEditor (LooperAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setResizeLimits (200, 400, 300, 600);
    setSize (150, 500);

    titleLabel.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    titleLabel.setText ("Looper", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (titleLabel);

    volumeSlider.setSliderStyle (juce::Slider::LinearVertical);
    volumeSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    volumeSlider.setRange (0.0, 1.0, 0.1);
    volumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "volume", volumeSlider);
    addAndMakeVisible (volumeSlider);

    volumeLabel.setFont (juce::FontOptions (12.0f));
    volumeLabel.setText ("Volume", juce::dontSendNotification);
    volumeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (volumeLabel);

    recordButton.setButtonText ("Record");
    recordButton.setColour (juce::TextButton::buttonColourId, juce::Colours::red);
    recordButton.setClickingTogglesState (true);
    recordAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.parameters, "record", recordButton);
    addAndMakeVisible (recordButton);
    audioProcessor.parameters.addParameterListener ("record", this);

    playButton.setButtonText ("Play");
    playButton.setColour (juce::TextButton::buttonColourId, juce::Colours::green);
    playButton.setClickingTogglesState (true);
    playAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.parameters, "play", playButton);
    addAndMakeVisible (playButton);
    audioProcessor.parameters.addParameterListener ("play", this);

    clearButton.setButtonText ("Undo All");
    clearButton.setColour (juce::TextButton::buttonColourId, juce::Colours::grey);
    clearButton.onClick = [this] { 
        audioProcessor.requestClearAll();
        // Wait a bit for audio thread to process, then update UI
        juce::Timer::callAfterDelay(50, [this] { updateLoopCount(); });
    };
    addAndMakeVisible (clearButton);

    undoButton.setButtonText ("Undo");
    undoButton.setColour (juce::TextButton::buttonColourId, juce::Colours::orange);
    undoButton.onClick = [this] { 
        audioProcessor.requestUndoLast();
        // Wait a bit for audio thread to process, then update UI
        juce::Timer::callAfterDelay(50, [this] { updateLoopCount(); });
    };
    addAndMakeVisible (undoButton);

    statusLabel.setFont (juce::FontOptions (12.0f));
    statusLabel.setText ("Ready", juce::dontSendNotification);
    statusLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (statusLabel);

    loopCountLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    loopCountLabel.setText ("Loops: 0", juce::dontSendNotification);
    loopCountLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (loopCountLabel);

    // trigger initial update
    parameterChanged("record", *audioProcessor.parameters.getRawParameterValue("record"));
    parameterChanged("play", *audioProcessor.parameters.getRawParameterValue("play"));
    
    // Set initial loop count
    int initialLoopCount = static_cast<int>(audioProcessor.getLoops().size());
    loopCountLabel.setText ("Loops: " + juce::String(initialLoopCount), juce::dontSendNotification);
}

LooperAudioProcessorEditor::~LooperAudioProcessorEditor()
{
    audioProcessor.parameters.removeParameterListener ("record", this);
    audioProcessor.parameters.removeParameterListener ("play", this);
    audioProcessor.parameters.removeParameterListener ("clear", this);
    audioProcessor.parameters.removeParameterListener ("undo", this);
}

void LooperAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey);
}

void LooperAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Title at the top (y: 0-30)
    titleLabel.setBounds (0, 0, bounds.getWidth(), 30);

    // Volume label (y: 40-60)
    volumeLabel.setBounds (0, 40, bounds.getWidth(), 20);

    // Volume slider (y: 60-220)
    auto sliderWidth = 60;
    auto sliderHeight = 160;
    auto sliderX = (bounds.getWidth() - sliderWidth) / 2;
    volumeSlider.setBounds (sliderX, 60, sliderWidth, sliderHeight);

    // Buttons
    int buttonWidth = juce::jmin(140, bounds.getWidth() - 20);
    int buttonHeight = 30;
    int buttonSpacing = 10;
    int buttonX = (bounds.getWidth() - buttonWidth) / 2;

    recordButton.setBounds (buttonX, 250, buttonWidth, buttonHeight);
    playButton.setBounds (buttonX, 250 + buttonHeight + buttonSpacing, buttonWidth, buttonHeight);
    clearButton.setBounds (buttonX, 250 + (buttonHeight + buttonSpacing) * 2, buttonWidth, buttonHeight);
    undoButton.setBounds (buttonX, 250 + (buttonHeight + buttonSpacing) * 3, buttonWidth, buttonHeight);

    // Status label (below buttons)
    statusLabel.setBounds (0, 400, bounds.getWidth(), 30);
    
    // Loop count label (at bottom)
    loopCountLabel.setBounds (0, 435, bounds.getWidth(), 25);
}

void LooperAudioProcessorEditor::updateLoopCount()
{
    int loopCount = static_cast<int>(audioProcessor.getLoops().size());
    loopCountLabel.setText ("Loops: " + juce::String(loopCount), juce::dontSendNotification);
}

void LooperAudioProcessorEditor::parameterChanged (const juce::String& parameterID, float newValue)
{
    juce::MessageManager::callAsync ([this, parameterID, newValue] {
        // Update loop count display
        updateLoopCount();

        if (parameterID == "record")
        {
            bool isRecording = newValue > 0.5f;
            if (isRecording)
            {
                recordButton.setButtonText ("Stop");
                recordButton.setColour (juce::TextButton::buttonColourId, juce::Colours::darkred);
                statusLabel.setText ("Recording...", juce::dontSendNotification);
            }
            else
            {
                recordButton.setButtonText ("Record");
                recordButton.setColour (juce::TextButton::buttonColourId, juce::Colours::red);

                // If not recording, check if playing
                if (playButton.getToggleState())
                    statusLabel.setText ("Playing...", juce::dontSendNotification);
                else
                    statusLabel.setText ("Ready", juce::dontSendNotification);
            }
        }
        else if (parameterID == "play")
        {
            bool isPlaying = newValue > 0.5f;
            if (isPlaying)
            {
                playButton.setButtonText ("Stop");
                playButton.setColour (juce::TextButton::buttonColourId, juce::Colours::darkgreen);
                statusLabel.setText ("Playing...", juce::dontSendNotification);
            }
            else
            {
                playButton.setButtonText ("Play");
                playButton.setColour (juce::TextButton::buttonColourId, juce::Colours::green);

                // If not playing, check if recording
                if (recordButton.getToggleState())
                    statusLabel.setText ("Recording...", juce::dontSendNotification);
                else
                    statusLabel.setText ("Ready", juce::dontSendNotification);
            }
        }
    });
}
