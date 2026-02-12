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
    parameters (*this, nullptr, "PARAMETERS", createParameterLayout()),
    writePosition (0),
    readPosition (0),
    isRecording (false),
    isPlaying (false),
    hasRecordedLoop (false),
    recordedLoopLength (0),
    currentSampleRate (44100.0),
    maxLoopLength (44100 * 60)
{
    volumeParam = parameters.getRawParameterValue ("volume");
    recordParam = parameters.getRawParameterValue ("record");
    playParam = parameters.getRawParameterValue ("play");
    clearParam = parameters.getRawParameterValue ("clear");
    
    startTimerHz(30);
}

LooperAudioProcessor::~LooperAudioProcessor()
{
    stopTimer();
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
    maxLoopLength = static_cast<int> (sampleRate * 60.0);

    loopBuffer.setSize (2, maxLoopLength);
    loopBuffer.clear();

    writePosition = 0;
    readPosition = 0;
    hasRecordedLoop = false;
    recordedLoopLength = 0;
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

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (clearParam->load() > 0.5f && !requestClear.load())
    {
        requestClear.store(true);
        loopBuffer.clear();
        writePosition = 0;
        readPosition = 0;
        hasRecordedLoop = false;
        recordedLoopLength = 0;
    }

    const auto shouldRecord = recordParam->load() > 0.5f;
    const auto shouldPlay = playParam->load() > 0.5f;

    // Check requestStopRecording to prevent re-triggering if we just stopped due to length
    if (shouldRecord && !isRecording && !requestStopRecording.load())
    {
        isRecording = true;
        writePosition = 0;
        loopBuffer.clear();
        hasRecordedLoop = false;
        recordedLoopLength = 0;
    }
    else if (!shouldRecord && isRecording)
    {
        stopRecording();
    }

    if (shouldPlay && !isPlaying)
    {
        isPlaying = true;
        readPosition = 0;
    }
    else if (!shouldPlay && isPlaying)
    {
        isPlaying = false;
    }

    if (isRecording)
    {
        int samplesToRecord = juce::jmin (numSamples, maxLoopLength - writePosition.load());

        for (int channel = 0; channel < numChannels; ++channel)
        {
            loopBuffer.copyFrom (channel, writePosition.load(), buffer, channel, 0, samplesToRecord);
        }

        writePosition += samplesToRecord;

        if (writePosition.load() >= maxLoopLength)
        {
            isRecording = false;
            // Unsafe: parameters.getParameterAsValue("record").setValue(false);
            requestStopRecording.store(true);
        }
    }

    if (isPlaying && hasRecordedLoop)
    {
        int loopLengthInSamples = recordedLoopLength;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            if (readPosition >= loopLengthInSamples)
                readPosition = 0;

            float volume = volumeParam->load();
            for (int channel = 0; channel < numChannels; ++channel)
            {
                float sampleValue = loopBuffer.getSample (channel % 2, readPosition) * volume;
                buffer.addSample (channel, sample, sampleValue);
            }

            readPosition++;
        }
    }
}

void LooperAudioProcessor::stopRecording()
{
    isRecording = false;
    int currentWritePos = writePosition.load();
    if (currentWritePos > 0)
    {
        hasRecordedLoop = true;
        recordedLoopLength = currentWritePos;

        // Apply Crossfade (approx 10ms)
        int fadeSamples = juce::jmin (recordedLoopLength, (int) (currentSampleRate * 0.01));

        if (fadeSamples > 0)
        {
            for (int channel = 0; channel < loopBuffer.getNumChannels(); ++channel)
            {
                auto* channelData = loopBuffer.getWritePointer (channel);
                for (int i = 0; i < fadeSamples; ++i)
                {
                    float alpha = (float) i / (float) fadeSamples;
                    int endSampleIdx = recordedLoopLength - fadeSamples + i;
                    // Mix start of loop into end of loop
                    channelData[endSampleIdx] = channelData[endSampleIdx] * (1.0f - alpha) + channelData[i] * alpha;
                }
            }
        }
    }
}

void LooperAudioProcessor::timerCallback()
{
    if (requestStopRecording.load())
    {
        parameters.getParameterAsValue("record").setValue(false);
        requestStopRecording.store(false);
    }
    
    if (requestClear.load())
    {
        auto clearValue = parameters.getParameterAsValue("clear");
        clearValue.setValue(false);
        requestClear.store(false);
    }
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
    juce::MemoryOutputStream stream (destData, true);
    state.writeToStream (stream);
}

void LooperAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ValueTree state = juce::ValueTree::readFromData (data, sizeInBytes);
    if (state.isValid())
    {
        parameters.replaceState (state);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout LooperAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat>("volume", "Volume", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));
    layout.add (std::make_unique<juce::AudioParameterBool>("record", "Record", false));
    layout.add (std::make_unique<juce::AudioParameterBool>("play", "Play", false));
    layout.add (std::make_unique<juce::AudioParameterBool>("clear", "Clear", false));

    return layout;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LooperAudioProcessor();
}
