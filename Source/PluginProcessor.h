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

#pragma once

#include "Models/Track.h"
#include "Models/TrackManager.h"
#include <juce_audio_processors/juce_audio_processors.h>

class LooperAudioProcessor
    : public juce::AudioProcessor,
      public juce::AudioProcessorValueTreeState::Listener {
public:
  LooperAudioProcessor();
  ~LooperAudioProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
#endif

  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
  using juce::AudioProcessor::processBlock;

  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override;

  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String &newName) override;

  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  juce::AudioProcessorValueTreeState parameters;

  // Track management for editor (delegated to TrackManager)
  Track *addTrack() { return trackManager.addTrack(); }
  void removeTrack(int trackId) { trackManager.removeTrack(trackId); }
  void removeAllTracks() { trackManager.removeAllTracks(); }
  std::vector<Track *> getTrackCopies() {
    return trackManager.getTrackCopies();
  }
  Track *findTrack(int trackId) { return trackManager.findTrack(trackId); }
  int getTrackCount() const { return trackManager.getTrackCount(); }

  // Track controls (delegated to TrackManager)
  void startRecordingTrack(int trackId);
  void stopRecordingTrack(int trackId) {
    trackManager.stopRecordingTrack(trackId);
  }
  void stopAllRecording() { trackManager.stopAllRecording(); }
  void startPlaybackTrack(int trackId) {
    trackManager.startPlaybackTrack(trackId);
  }
  void stopPlaybackTrack(int trackId) {
    trackManager.stopPlaybackTrack(trackId);
  }
  void clearTrack(int trackId) { trackManager.clearTrack(trackId); }
  void undoTrack(int trackId) { trackManager.undoTrack(trackId); }

  // Global controls (delegated to TrackManager)
  void requestClearAll() { trackManager.requestClearAll(); }
  void requestUndoLast() { trackManager.requestUndoLast(); }
  void startPlayback() { trackManager.startPlayback(); }
  void stopPlayback() { trackManager.stopPlayback(); }
  bool isPlaying() const { return trackManager.isPlaying(); }

  // Solo logic (delegated to TrackManager)
  bool isAnyTrackSoloed() const { return trackManager.isAnyTrackSoloed(); }

  // Access to track manager
  TrackManager &getTrackManager() { return trackManager; }

  // Current track for host automation
  void setCurrentTrackId(int trackId) {
    currentTrackId = trackId;
    syncParamsWithCurrentTrack();
  }
  int getCurrentTrackId() const { return currentTrackId; }
  void syncParamsWithCurrentTrack();

  // AudioProcessorValueTreeState::Listener
  void parameterChanged(const juce::String &parameterID,
                        float newValue) override;

private:
  std::atomic<float> *playAllParam = nullptr;
  std::atomic<float> *monitorParam = nullptr;
  std::atomic<float> *recordParam = nullptr;
  std::atomic<float> *playParam = nullptr;
  std::atomic<float> *soloParam = nullptr;
  std::atomic<float> *clearParam = nullptr;
  std::atomic<float> *undoParam = nullptr;
  std::atomic<int> currentTrackId{-1};

  // Core components
  TrackManager trackManager;

  // State
  double currentSampleRate = 44100.0;

  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LooperAudioProcessor)
};
