#pragma once

#include "Audio/AudioGraphManager.h"
#include "Core/MidiPlayer.h"
#include "Core/NcnParser.h"
#include "Core/QueueManager.h"
#include "Core/Routing/MixerController.h"
#include <JuceHeader.h>

class KaraokeEngine : public juce::AudioProcessor, public juce::AsyncUpdater {
public:
  KaraokeEngine();
  ~KaraokeEngine() override;

  //==============================================================================
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

  //==============================================================================
  juce::AudioProcessorEditor *createEditor() override { return nullptr; }
  bool hasEditor() const override { return false; }

  const juce::String getName() const override { return "Karaoke Engine"; }
  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int index) override {}
  const juce::String getProgramName(int index) override { return ""; }
  void changeProgramName(int index, const juce::String &newName) override {}

  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  //==============================================================================
  // Karaoke Actions
  bool loadSong(const juce::File &midiFile, const juce::File &lyrFile,
                const juce::File &curFile);
  void play();
  void pause();
  void stop();
  void nextTrack(); // Manual skip or auto skip

  // SF2
  void loadSoundFont(const juce::File &sf2File);

  // Getters for UI
  MidiPlayer &getMidiPlayer() { return midiPlayer; }
  MixerController &getMixerController() { return mixerController; }
  AudioGraphManager &getGraphManager() { return graphManager; }
  QueueManager &getQueueManager() { return queueManager; }
  const SongLyrics &getCurrentLyrics() const { return currentLyrics; }

  std::function<void()> onSongLoaded;

private:
  void handleAsyncUpdate() override;

  MidiPlayer midiPlayer;
  MixerController mixerController;
  AudioGraphManager graphManager;
  NcnParser ncnParser;
  QueueManager queueManager;

  SongLyrics currentLyrics;

  bool wasPlaying = false;
  void checkQueue();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KaraokeEngine)
};
