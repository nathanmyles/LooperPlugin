/*
 * LooperPlugin
 * Copyright (C) 2026 NathanMyles
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "PluginProcessor.h"
#include "PluginEditor.h"

LooperAudioProcessor::LooperAudioProcessor()
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
              ),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout()) {
  playAllParam = parameters.getRawParameterValue("playAll");
  monitorParam = parameters.getRawParameterValue("monitor");
  recordParam = parameters.getRawParameterValue("record");
  playParam = parameters.getRawParameterValue("play");
  soloParam = parameters.getRawParameterValue("solo");
  clearParam = parameters.getRawParameterValue("clear");
  undoParam = parameters.getRawParameterValue("undo");

  parameters.addParameterListener("playAll", this);
  parameters.addParameterListener("record", this);
  parameters.addParameterListener("play", this);
  parameters.addParameterListener("solo", this);
  parameters.addParameterListener("clear", this);
  parameters.addParameterListener("undo", this);
}

LooperAudioProcessor::~LooperAudioProcessor() {
  parameters.removeParameterListener("playAll", this);
  parameters.removeParameterListener("record", this);
  parameters.removeParameterListener("play", this);
  parameters.removeParameterListener("solo", this);
  parameters.removeParameterListener("clear", this);
  parameters.removeParameterListener("undo", this);
}

const juce::String LooperAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool LooperAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool LooperAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool LooperAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double LooperAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int LooperAudioProcessor::getNumPrograms() { return 1; }

int LooperAudioProcessor::getCurrentProgram() { return 0; }

void LooperAudioProcessor::setCurrentProgram(int index) {
  juce::ignoreUnused(index);
}

const juce::String LooperAudioProcessor::getProgramName(int index) {
  juce::ignoreUnused(index);
  return {};
}

void LooperAudioProcessor::changeProgramName(int index,
                                             const juce::String &newName) {
  juce::ignoreUnused(index, newName);
}

void LooperAudioProcessor::prepareToPlay(double sampleRate,
                                         int samplesPerBlock) {
  juce::ignoreUnused(samplesPerBlock);
  currentSampleRate = sampleRate;
  trackManager.prepare(sampleRate);
}

void LooperAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool LooperAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
#if JucePlugin_IsMidiEffect
  juce::ignoreUnused(layouts);
  return true;
#else
  const auto &mainOutput = layouts.getMainOutputChannelSet();
  const auto &mainInput = layouts.getMainInputChannelSet();

  if (mainOutput != mainInput)
    return false;

  if (mainOutput.size() > 2)
    return false;

  return true;
#endif
}
#endif

void LooperAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                        juce::MidiBuffer &midiMessages) {
  juce::ignoreUnused(midiMessages);

  const auto shouldMonitor = monitorParam->load() > 0.5f;

  trackManager.processBlock(buffer, shouldMonitor);
}

void LooperAudioProcessor::startRecordingTrack(int trackId) {
  bool autoStarted = trackManager.startRecordingTrack(trackId);
  if (autoStarted) {
    parameters.getParameter("playAll")->setValueNotifyingHost(1.0f);
  }
}

bool LooperAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor *LooperAudioProcessor::createEditor() {
  return new LooperAudioProcessorEditor(*this);
}

void LooperAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
  juce::ValueTree state = parameters.copyState();

  trackManager.getState(state, currentSampleRate);

  juce::MemoryOutputStream stream(destData, true);
  state.writeToStream(stream);
}

void LooperAudioProcessor::setStateInformation(const void *data,
                                               int sizeInBytes) {
  juce::ValueTree state =
      juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));
  if (state.isValid()) {
    parameters.replaceState(state);

    trackManager.setState(state, currentSampleRate);
  }
}

juce::AudioProcessorValueTreeState::ParameterLayout
LooperAudioProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  layout.add(
      std::make_unique<juce::AudioParameterBool>("playAll", "Play All", false));
  layout.add(
      std::make_unique<juce::AudioParameterBool>("monitor", "Monitor", false));
  layout.add(
      std::make_unique<juce::AudioParameterBool>("record", "Record", false));
  layout.add(std::make_unique<juce::AudioParameterBool>("play", "Play", false));
  layout.add(
      std::make_unique<juce::AudioParameterBool>("solo", "Solo", false));
  layout.add(
      std::make_unique<juce::AudioParameterBool>("clear", "Clear", false));
  layout.add(
      std::make_unique<juce::AudioParameterBool>("undo", "Undo", false));

  return layout;
}

void LooperAudioProcessor::parameterChanged(const juce::String &parameterID,
                                            float newValue) {
  if (parameterID == "playAll") {
    if (newValue >= 0.5f)
      startPlayback();
    else
      stopPlayback();
    return;
  }

  if (parameterID == "monitor")
    return;

  auto *track = findTrack(currentTrackId);
  if (track == nullptr)
    return;

  if (parameterID == "record") {
    bool shouldBeRecording = newValue >= 0.5f;
    if (track->isRecording() == shouldBeRecording)
      return;
    if (shouldBeRecording)
      trackManager.startRecordingTrack(currentTrackId);
    else
      trackManager.stopRecordingTrack(currentTrackId);
  } else if (parameterID == "play") {
    bool shouldBePlaying = newValue >= 0.5f;
    if (track->isPlaying() == shouldBePlaying)
      return;
    if (shouldBePlaying)
      trackManager.startPlaybackTrack(currentTrackId);
    else
      trackManager.stopPlaybackTrack(currentTrackId);
  } else if (parameterID == "solo") {
    bool shouldBeSoloed = newValue >= 0.5f;
    if (track->isSoloed() == shouldBeSoloed)
      return;
    track->setSoloed(shouldBeSoloed);
  } else if (parameterID == "clear" && newValue >= 0.5f) {
    clearTrack(currentTrackId);
    if (auto *param = parameters.getParameter("clear"))
      param->setValueNotifyingHost(0.0f);
  } else if (parameterID == "undo" && newValue >= 0.5f) {
    undoTrack(currentTrackId);
    if (auto *param = parameters.getParameter("undo"))
      param->setValueNotifyingHost(0.0f);
  }
}

void LooperAudioProcessor::syncParamsWithCurrentTrack() {
  auto *track = findTrack(currentTrackId);
  if (track == nullptr) {
    if (auto *p = parameters.getParameter("record"))
      p->setValueNotifyingHost(0.0f);
    if (auto *p = parameters.getParameter("play"))
      p->setValueNotifyingHost(0.0f);
    if (auto *p = parameters.getParameter("solo"))
      p->setValueNotifyingHost(0.0f);
    return;
  }
  if (auto *p = parameters.getParameter("record"))
    p->setValueNotifyingHost(track->isRecording() ? 1.0f : 0.0f);
  if (auto *p = parameters.getParameter("play"))
    p->setValueNotifyingHost(track->isPlaying() ? 1.0f : 0.0f);
  if (auto *p = parameters.getParameter("solo"))
    p->setValueNotifyingHost(track->isSoloed() ? 1.0f : 0.0f);
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new LooperAudioProcessor();
}
