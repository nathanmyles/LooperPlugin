#include "PluginProcessor.h"
#include "PluginEditor.h"

LooperAudioProcessorEditor::LooperAudioProcessorEditor (LooperAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setResizeLimits (300, 200, 600, 400);
    setSize (400, 250);

    titleLabel.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    titleLabel.setText ("Looper", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (titleLabel);

    loopLengthSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    loopLengthSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    loopLengthSlider.setRange (0.1, 10.0, 0.1);
    loopLengthSlider.setValue (2.0);
    loopLengthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
    audioProcessor.parameters, "loopLength", loopLengthSlider);
    addAndMakeVisible (loopLengthSlider);

    loopLengthLabel.setFont (juce::FontOptions (12.0f));
    loopLengthLabel.setText ("Loop Length (sec)", juce::dontSendNotification);
    loopLengthLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (loopLengthLabel);

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
    
    auto titleArea = bounds.removeFromTop (40);
    titleLabel.setBounds (titleArea);
    
    auto controlArea = bounds.removeFromTop (60);
    auto sliderArea = controlArea.removeFromTop (40);
    loopLengthLabel.setBounds (sliderArea.removeFromTop (15));
    loopLengthSlider.setBounds (sliderArea);
    
    auto buttonArea = bounds.removeFromTop (50);
    int buttonWidth = 80;
    int buttonSpacing = 20;
    int totalButtonWidth = buttonWidth * 3 + buttonSpacing * 2;
    int startX = (buttonArea.getWidth() - totalButtonWidth) / 2;
    
    recordButton.setBounds (startX, 10, buttonWidth, 30);
    playButton.setBounds (startX + buttonWidth + buttonSpacing, 10, buttonWidth, 30);
    clearButton.setBounds (startX + (buttonWidth + buttonSpacing) * 2, 10, buttonWidth, 30);
    
    statusLabel.setBounds (bounds);
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