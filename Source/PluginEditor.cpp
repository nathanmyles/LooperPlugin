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
    recordButton.addListener (this);
    addAndMakeVisible (recordButton);
    recordParam = audioProcessor.parameters.getParameter ("record");

    playButton.setButtonText ("Play");
    playButton.setColour (juce::TextButton::buttonColourId, juce::Colours::green);
    playButton.addListener (this);
    addAndMakeVisible (playButton);
    playParam = audioProcessor.parameters.getParameter ("play");

    clearButton.setButtonText ("Clear");
    clearButton.setColour (juce::TextButton::buttonColourId, juce::Colours::grey);
    clearButton.addListener (this);
    addAndMakeVisible (clearButton);
    clearParam = audioProcessor.parameters.getParameter ("clear");

    statusLabel.setFont (juce::FontOptions (12.0f));
    statusLabel.setText ("Ready", juce::dontSendNotification);
    statusLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (statusLabel);
}

LooperAudioProcessorEditor::~LooperAudioProcessorEditor()
{
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
    
    // Buttons (y: 220-340)
    int buttonWidth = 180;
    int buttonHeight = 30;
    int buttonSpacing = 10;
    int buttonX = (bounds.getWidth() - buttonWidth) / 2;
    
    recordButton.setBounds (buttonX, 250, buttonWidth, buttonHeight);
    playButton.setBounds (buttonX, 250 + buttonHeight + buttonSpacing, buttonWidth, buttonHeight);
    clearButton.setBounds (buttonX, 250 + (buttonHeight + buttonSpacing) * 2, buttonWidth, buttonHeight);
    
    // Status label (y: 340-500)
    statusLabel.setBounds (0, 340, bounds.getWidth(), bounds.getHeight() - 340);
}

void LooperAudioProcessorEditor::buttonClicked (juce::Button* button)
{
    if (button == &recordButton)
    {
        bool isRecording = recordParam->getValue() > 0.5f;
        audioProcessor.getParameters().getUnchecked (1)->setValueNotifyingHost (!isRecording);
        
        if (isRecording)
        {
            recordButton.setButtonText ("Record");
            recordButton.setColour (juce::TextButton::buttonColourId, juce::Colours::red);
            statusLabel.setText ("Ready", juce::dontSendNotification);
        }
        else
        {
            recordButton.setButtonText ("Stop");
            recordButton.setColour (juce::TextButton::buttonColourId, juce::Colours::darkred);
            statusLabel.setText ("Recording...", juce::dontSendNotification);
        }
    }
    else if (button == &playButton)
    {
        bool isPlaying = playParam->getValue() > 0.5f;
        if (playParam)
        {
            playParam->beginChangeGesture();
            playParam->setValueNotifyingHost(!isPlaying ? 1.0f : 0.0f);
            playParam->endChangeGesture();
        }
        
        if (isPlaying)
        {
            playButton.setButtonText ("Play");
            playButton.setColour (juce::TextButton::buttonColourId, juce::Colours::green);
            statusLabel.setText ("Ready", juce::dontSendNotification);
        }
        else
        {
            playButton.setButtonText ("Stop");
            playButton.setColour (juce::TextButton::buttonColourId, juce::Colours::darkgreen);
            statusLabel.setText ("Playing...", juce::dontSendNotification);
        }
    }
    else if (button == &clearButton)
    {
        if (clearParam)
        {
            clearParam->beginChangeGesture();
            clearParam->setValueNotifyingHost(1.0f);
            clearParam->endChangeGesture();
        }
        statusLabel.setText ("Cleared", juce::dontSendNotification);
    }
}