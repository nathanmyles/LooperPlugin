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

    clearButton.setButtonText ("Clear");
    clearButton.setColour (juce::TextButton::buttonColourId, juce::Colours::grey);
    clearButton.setClickingTogglesState (true);
    clearAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.parameters, "clear", clearButton);
    addAndMakeVisible (clearButton);
    audioProcessor.parameters.addParameterListener ("clear", this);

    statusLabel.setFont (juce::FontOptions (12.0f));
    statusLabel.setText ("Ready", juce::dontSendNotification);
    statusLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (statusLabel);

    // trigger initial update
    parameterChanged("record", *audioProcessor.parameters.getRawParameterValue("record"));
    parameterChanged("play", *audioProcessor.parameters.getRawParameterValue("play"));
}

LooperAudioProcessorEditor::~LooperAudioProcessorEditor()
{
    audioProcessor.parameters.removeParameterListener ("record", this);
    audioProcessor.parameters.removeParameterListener ("play", this);
    audioProcessor.parameters.removeParameterListener ("clear", this);
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

    // Status label (below buttons)
    statusLabel.setBounds (0, 370, bounds.getWidth(), bounds.getHeight() - 370);
}

void LooperAudioProcessorEditor::parameterChanged (const juce::String& parameterID, float newValue)
{
    juce::MessageManager::callAsync ([this, parameterID, newValue] {
        // Use a SafePointer if needed, but for now we assume simple lifecycle
        // Check if the editor is still valid?
        // In a real plugin, use SafePointer<LooperAudioProcessorEditor> safeThis (this);
        // if (safeThis == nullptr) return;

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
        else if (parameterID == "clear")
        {
             if (newValue > 0.5f) statusLabel.setText ("Cleared", juce::dontSendNotification);
        }
    });
}
