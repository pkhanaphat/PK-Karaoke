#pragma once
#include "Audio/tsf.h"
#include "Core/MidiHelper.h"
#include "Core/Routing/MixerController.h"
#include <JuceHeader.h>
#include <optional>

/**
 * SF2Source — SoundFont (.sf2) Instrument สำหรับ PK Karaoke
 * Extends juce::AudioProcessor เพื่อใช้ใน AudioProcessorGraph ได้โดยตรง
 */
class SF2Source : public juce::AudioProcessor {
public:
  explicit SF2Source(MixerController *mixer = nullptr,
                     std::optional<InstrumentGroup> targetGroup = std::nullopt);
  ~SF2Source() override;

  //==========================================================================
  // Instrument API
  bool loadSoundFont(const juce::File &sf2File, tsf *sharedSynth = nullptr);
  bool isLoaded() const { return mainSynth != nullptr; }
  juce::String getLoadedPath() const { return loadedPath; }
  void setSFVolume(float linearGain);
  void setMixerController(MixerController *mc) { mixer = mc; }
  void setTargetGroup(std::optional<InstrumentGroup> group) {
    targetGroup = group;
  }

  //==========================================================================
  // AudioProcessor interface
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) override;

  juce::AudioProcessorEditor *createEditor() override { return nullptr; }
  bool hasEditor() const override { return false; }
  const juce::String getName() const override { return "SF2Source"; }
  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }
  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String &) override {}
  void getStateInformation(juce::MemoryBlock &) override {}
  void setStateInformation(const void *, int) override {}

private:
  tsf *mainSynth = nullptr;
  tsf *channelSynths[16] = {nullptr};
  int channelPrograms[16] = {0};
  bool isDrumChannel[16] = {false};

  MixerController *mixer = nullptr;
  float currentVolume = 1.0f;
  juce::String loadedPath;

  juce::CriticalSection lock;
  double currentSampleRate = 44100.0;
  int maxSamplesPerBlock = 1024;
  juce::HeapBlock<float> interleavedBuffer;

  std::optional<InstrumentGroup> targetGroup; // Only play this group
  std::map<InstrumentGroup, tsf *> drumSynths;

  void freeSynths();
  void renderChannels(juce::AudioBuffer<float> &dest, int startSample,
                      int numSamples);
  void processMidiMessage(const juce::MidiMessage &msg);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SF2Source)
};
