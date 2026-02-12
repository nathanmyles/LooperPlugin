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
    isPlaying (false),
    currentSampleRate (44100.0),
    maxLoopLength (44100 * 60)
{
    volumeParam = parameters.getRawParameterValue ("volume");
    recordParam = parameters.getRawParameterValue ("record");
    playParam = parameters.getRawParameterValue ("play");

    startTimerHz(60);  // Increased from 30Hz for faster button reset
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

    // Clear all loops
    loops.clear();
    baseLoopLength = 0;
    recordingLoopIndex = -1;
    writePosition = 0;
    readPosition = 0;
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

void LooperAudioProcessor::addNewLoop()
{
    auto newLoop = std::make_unique<Loop>();
    newLoop->buffer.setSize(2, maxLoopLength);
    newLoop->buffer.clear();
    newLoop->length = 0;
    newLoop->hasContent = false;
    loops.push_back(std::move(newLoop));
}

void LooperAudioProcessor::removeLastLoop()
{
    if (!loops.empty())
    {
        // If currently recording this loop, cancel recording and reset record button
        if (recordingLoopIndex == static_cast<int>(loops.size()) - 1)
        {
            recordingLoopIndex = -1;
            writePosition = 0;
            // Request record button reset on message thread
            requestStopRecording.store(true);
        }
        loops.pop_back();

        // If we removed the last loop, reset base length
        if (loops.empty())
        {
            baseLoopLength = 0;
        }
    }
}

void LooperAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Handle clear action (triggered from UI)
    if (requestClear.exchange(false))
    {
        loops.clear();
        baseLoopLength = 0;
        recordingLoopIndex = -1;
        writePosition = 0;
        readPosition = 0;
    }

    // Handle undo action (triggered from UI)
    if (requestUndo.exchange(false))
    {
        removeLastLoop();
    }

    const auto shouldRecord = recordParam->load() > 0.5f;
    const auto shouldPlay = playParam->load() > 0.5f;

    // Handle record toggle
    if (shouldRecord && recordingLoopIndex == -1 && !requestStopRecording.load())
    {
        // Start recording - create new loop
        addNewLoop();
        recordingLoopIndex = static_cast<int>(loops.size()) - 1;
        writePosition = 0;

        // If this is the first loop, we're setting the base length
        // If not, we'll sync to baseLoopLength
    }
    else if (!shouldRecord && recordingLoopIndex != -1)
    {
        // Stop recording
        stopRecording(recordingLoopIndex);
        recordingLoopIndex = -1;
    }

    // Handle play toggle
    if (shouldPlay && !isPlaying)
    {
        isPlaying = true;
        readPosition = 0;
    }
    else if (!shouldPlay && isPlaying)
    {
        isPlaying = false;
    }

    // Recording
    if (recordingLoopIndex != -1)
    {
        auto& currentLoop = loops[static_cast<size_t>(recordingLoopIndex)];
        int maxRecordLength = (baseLoopLength > 0) ? baseLoopLength : maxLoopLength;
        int samplesToRecord = juce::jmin(numSamples, maxRecordLength - writePosition.load());

        if (samplesToRecord > 0)
        {
            for (int channel = 0; channel < numChannels; ++channel)
            {
                currentLoop->buffer.copyFrom(channel, writePosition.load(), buffer, channel, 0, samplesToRecord);
            }

            writePosition += samplesToRecord;
            currentLoop->length = writePosition.load();

            // If we've hit the base loop length or max, stop
            if (writePosition.load() >= maxRecordLength && baseLoopLength > 0)
            {
                requestStopRecording.store(true);
            }
            else if (writePosition.load() >= maxLoopLength)
            {
                requestStopRecording.store(true);
            }
        }
    }

    // Playback - mix all loops
    if (isPlaying && !loops.empty())
    {
        float volume = volumeParam->load();

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Wrap read position to base loop length
            int readPos = readPosition.load();
            if (baseLoopLength > 0 && readPos >= baseLoopLength)
            {
                readPos = 0;
                readPosition.store(0);
            }

            for (int channel = 0; channel < numChannels; ++channel)
            {
                float mixedSample = 0.0f;

                // Mix all loops
                for (auto& loop : loops)
                {
                    if (loop->hasContent)
                    {
                        // Wrap to individual loop length
                        int loopReadPos = readPos % loop->length;
                        mixedSample += loop->buffer.getSample(channel % 2, loopReadPos);
                    }
                }

                buffer.addSample(channel, sample, mixedSample * volume);
            }

            readPosition++;
        }
    }
}

void LooperAudioProcessor::stopRecording(int loopIndex)
{
    if (loopIndex >= 0 && loopIndex < static_cast<int>(loops.size()))
    {
        auto& loop = loops[static_cast<size_t>(loopIndex)];
        int currentWritePos = writePosition.load();

        if (currentWritePos > 0)
        {
            loop->hasContent = true;
            loop->length = currentWritePos;

            // If this is the first loop, set base length
            if (baseLoopLength == 0)
            {
                baseLoopLength = loop->length;
            }

            // Apply Crossfade (approx 10ms)
            int fadeSamples = juce::jmin(loop->length, (int)(currentSampleRate * 0.01));

            if (fadeSamples > 0)
            {
                for (int channel = 0; channel < loop->buffer.getNumChannels(); ++channel)
                {
                    auto* channelData = loop->buffer.getWritePointer(channel);
                    for (int i = 0; i < fadeSamples; ++i)
                    {
                        float alpha = (float)i / (float)fadeSamples;
                        int endSampleIdx = loop->length - fadeSamples + i;
                        // Mix start of loop into end of loop
                        channelData[endSampleIdx] = channelData[endSampleIdx] * (1.0f - alpha) + channelData[i] * alpha;
                    }
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
}

void LooperAudioProcessor::requestClearAll()
{
    requestClear.store(true);
}

void LooperAudioProcessor::requestUndoLast()
{
    requestUndo.store(true);
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

    // Add loop count
    state.setProperty("loopCount", static_cast<int>(loops.size()), nullptr);
    state.setProperty("baseLoopLength", baseLoopLength, nullptr);

    // Save each loop's audio data
    for (size_t i = 0; i < loops.size(); ++i)
    {
        juce::String loopKey = "loop_" + juce::String(i);
        auto& loop = loops[i];

        juce::MemoryBlock loopData;
        juce::MemoryOutputStream loopStream(loopData, true);

        // Write loop metadata
        loopStream.writeInt(loop->length);
        loopStream.writeBool(loop->hasContent);

        // Write audio data
        if (loop->hasContent && loop->length > 0)
        {
            for (int channel = 0; channel < loop->buffer.getNumChannels(); ++channel)
            {
                loopStream.write(loop->buffer.getReadPointer(channel),
                                sizeof(float) * static_cast<size_t>(loop->length));
            }
        }

        state.setProperty(loopKey, loopData.toBase64Encoding(), nullptr);
    }

    juce::MemoryOutputStream stream (destData, true);
    state.writeToStream (stream);
}

void LooperAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ValueTree state = juce::ValueTree::readFromData (data, static_cast<size_t>(sizeInBytes));
    if (state.isValid())
    {
        parameters.replaceState (state);

        // Restore loops
        int loopCount = state.getProperty("loopCount", 0);
        baseLoopLength = state.getProperty("baseLoopLength", 0);

        loops.clear();

        for (int i = 0; i < loopCount; ++i)
        {
            juce::String loopKey = "loop_" + juce::String(i);
            juce::String loopDataBase64 = state.getProperty(loopKey, "");

            if (loopDataBase64.isNotEmpty())
            {
                juce::MemoryBlock loopData;
                loopData.fromBase64Encoding(loopDataBase64);
                juce::MemoryInputStream loopStream(loopData, false);

                auto newLoop = std::make_unique<Loop>();
                newLoop->buffer.setSize(2, maxLoopLength);
                newLoop->buffer.clear();

                newLoop->length = loopStream.readInt();
                newLoop->hasContent = loopStream.readBool();

                if (newLoop->hasContent && newLoop->length > 0)
                {
                    for (int channel = 0; channel < newLoop->buffer.getNumChannels(); ++channel)
                    {
                        loopStream.read(newLoop->buffer.getWritePointer(channel),
                                       static_cast<int>(sizeof(float) * static_cast<size_t>(newLoop->length)));
                    }
                }

                loops.push_back(std::move(newLoop));
            }
        }
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout LooperAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat>("volume", "Volume", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));
    layout.add (std::make_unique<juce::AudioParameterBool>("record", "Record", false));
    layout.add (std::make_unique<juce::AudioParameterBool>("play", "Play", false));

    return layout;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LooperAudioProcessor();
}
