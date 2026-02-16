#include "PluginProcessor.h"
#include "PluginEditor.h"

LooperAudioProcessorEditor::LooperAudioProcessorEditor(LooperAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      looperView(audioProcessor.parameters, audioProcessor.getLoops())
{
    setResizeLimits(200, 400, 300, 600);
    setSize(150, 500);

    // Set up callbacks for clear and undo actions
    looperView.onClearAll = [this] {
        audioProcessor.requestClearAll();
    };

    looperView.onUndoLast = [this] {
        audioProcessor.requestUndoLast();
    };

    addAndMakeVisible(looperView);
}

LooperAudioProcessorEditor::~LooperAudioProcessorEditor()
{
}

void LooperAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
}

void LooperAudioProcessorEditor::resized()
{
    // LooperView takes up the entire editor area
    looperView.setBounds(getLocalBounds());
}
