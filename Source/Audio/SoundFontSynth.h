#pragma once

#include "Audio/tsf.h"
#include "Core/MidiHelper.h"
#include "Core/Routing/MixerController.h"
#include <JuceHeader.h>

class SoundFontSynth : public juce::AudioProcessor {
public:
  SoundFontSynth(MixerController *mixerController = nullptr);
  ~SoundFontSynth() override;

  //==============================================================================
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

  //==============================================================================
  juce::AudioProcessorEditor *createEditor() override { return nullptr; }
  bool hasEditor() const override { return false; }

  const juce::String getName() const override { return "SoundFont Synth"; }
  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int index) override {}
  const juce::String getProgramName(int index) override { return ""; }
  void changeProgramName(int index, const juce::String &newName) override {}

  //==============================================================================
  void getStateInformation(juce::MemoryBlock &destData) override {}
  void setStateInformation(const void *data, int sizeInBytes) override {}

  //==============================================================================
  bool loadSoundFont(const juce::File &sf2File);
  void setVolume(float newVolume);

  void setMixerController(MixerController *mc) { mixer = mc; }

private:
  tsf *mainSynth = nullptr;
  int channelPrograms[16] = {0}; // Track active program per channel
  bool isDrumChannel[16] = {false};

  MixerController *mixer = nullptr;
  float currentVolume = 1.0f;
  juce::SpinLock lock;
  double currentSampleRate = 44100.0;
  juce::HeapBlock<float> interleavedBuffer;
  int maxSamplesPerBlock = 1024;

  void freeSynths();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundFontSynth)
};
