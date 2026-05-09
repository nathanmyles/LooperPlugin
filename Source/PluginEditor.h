#pragma once

#include "PluginProcessor.h"
#include "Views/GlobalControlBar.h"
#include "Views/TrackContainer.h"

class LooperAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   public juce::Timer {
public:
  LooperAudioProcessorEditor(LooperAudioProcessor &);
  ~LooperAudioProcessorEditor() override;

  void paint(juce::Graphics &) override;
  void resized() override;

  // Timer callback for UI updates
  void timerCallback() override;

private:
  LooperAudioProcessor &audioProcessor;

  // UI Components
  GlobalControlBar controlBar;
  TrackContainer trackContainer;

  void setupCallbacks();
  void addInitialTrack();
  void updateTrackButtons();
  void syncTracksWithProcessor();
  bool keyPressed(const juce::KeyPress &key) override;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LooperAudioProcessorEditor)
};
