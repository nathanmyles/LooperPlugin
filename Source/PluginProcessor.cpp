#include "PluginProcessor.h"
#include "PluginEditor.h"

LooperAudioProcessor::LooperAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    volumeParam = parameters.getRawParameterValue ("volume");
    recordParam = parameters.getRawParameterValue ("record");
    playParam = parameters.getRawParameterValue ("play");
    monitorParam = parameters.getRawParameterValue ("monitor");
}

LooperAudioProcessor::~LooperAudioProcessor()
{
}

const juce::String LooperAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool LooperAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool LooperAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool LooperAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double LooperAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int LooperAudioProcessor::getNumPrograms()
{
    return 1;
}

int LooperAudioProcessor::getCurrentProgram()
{
    return 0;
}

void LooperAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused(index);
}

const juce::String LooperAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused(index);
    return {};
}

void LooperAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void LooperAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    currentSampleRate = sampleRate;
    looper.prepare(sampleRate);
}

void LooperAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool LooperAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    const auto& mainOutput = layouts.getMainOutputChannelSet();
    const auto& mainInput  = layouts.getMainInputChannelSet();

    if (mainOutput != mainInput)
        return false;

    if (mainOutput.size() > 2)
        return false;

    return true;
  #endif
}
#endif

void LooperAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    // Handle pending requests from UI thread
    looper.handlePendingRequests();

    const auto shouldRecord = recordParam->load() > 0.5f;
    const auto shouldPlay = playParam->load() > 0.5f;
    const auto shouldMonitor = monitorParam->load() > 0.5f;

    // Handle record toggle
    if (shouldRecord && !looper.isRecording())
    {
        looper.startRecording();
    }
    else if (!shouldRecord && looper.isRecording())
    {
        looper.stopRecording();
    }

    // Handle play toggle
    if (shouldPlay && !looper.isPlaying())
    {
        looper.startPlayback();
    }
    else if (!shouldPlay && looper.isPlaying())
    {
        looper.stopPlayback();
    }

    // Recording
    looper.processRecording(buffer);

    // Playback - mix all loops
    float volume = volumeParam->load();

    // If not monitoring, clear input before playback to prevent feedback
    if (!shouldMonitor)
    {
        buffer.clear();
    }

    looper.processPlayback(buffer, volume);
}

void LooperAudioProcessor::requestClearAll()
{
    looper.requestClearAll();
}

void LooperAudioProcessor::requestUndoLast()
{
    looper.requestUndoLast();
}

bool LooperAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* LooperAudioProcessor::createEditor()
{
    return new LooperAudioProcessorEditor (*this);
}

void LooperAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree state = parameters.copyState();
    looper.getState(state, currentSampleRate);

    juce::MemoryOutputStream stream (destData, true);
    state.writeToStream (stream);
}

void LooperAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ValueTree state = juce::ValueTree::readFromData (data, static_cast<size_t>(sizeInBytes));
    if (state.isValid())
    {
        parameters.replaceState (state);
        looper.setState(state, currentSampleRate);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout LooperAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat>("volume", "Volume", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));
    layout.add (std::make_unique<juce::AudioParameterBool>("record", "Record", false));
    layout.add (std::make_unique<juce::AudioParameterBool>("play", "Play", false));
    layout.add (std::make_unique<juce::AudioParameterBool>("monitor", "Monitor", false));

    return layout;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LooperAudioProcessor();
}
