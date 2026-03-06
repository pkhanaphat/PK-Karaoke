#pragma once
#include "Audio/tsf.h"
#include "Core/MidiHelper.h"
#include "Core/Routing/MixerController.h"
#include <JuceHeader.h>
#include <functional>
#include <optional>
#include <unordered_set>

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

  void setExcludedGroups(const std::vector<InstrumentGroup> &groups) {
    const juce::ScopedLock sl(lock);
    excludedGroups.clear();
    for (auto g : groups)
      excludedGroups.insert(static_cast<int>(g));
  }

  void updateCustomSF2Routing(
      const std::map<InstrumentGroup, juce::String> &customPaths,
      std::function<tsf *(const juce::String &)> getSharedFontCallback);

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
  int channelPrograms[16] = {0};
  bool isDrumChannel[16] = {false};
  int noteOriginGroup[16][128]; // [channel][noteNumber] = group index

  MixerController *mixer = nullptr;
  float currentVolume = 1.0f;
  juce::String loadedPath;

  juce::CriticalSection lock;
  double currentSampleRate = 44100.0;
  int maxSamplesPerBlock = 1024;
  juce::HeapBlock<float> interleavedBuffer;
  juce::AudioBuffer<float> internalMixBuffer;

  std::optional<InstrumentGroup> targetGroup; // Only play this group
  // Audio thread excluded group set — use int as key for unordered_set
  std::unordered_set<int> excludedGroups;

  std::map<InstrumentGroup, tsf *>
      activeSynths; // Dynamically allocated synths based on playing notes
  std::map<InstrumentGroup, tsf *> customSynths; // For custom SF2 per-track

  // Pre-allocated render cache to avoid heap allocation every audio callback
  std::vector<InstrumentGroup> groupsToRenderCache;
  std::array<std::pair<float, float>, 256>
      groupPeaksCache{}; // indexed by (int)group

  void freeSynths();
  tsf *getOrCreateSynthForGroup(InstrumentGroup group,
                                tsf *sharedFontToCopyFrom);

  void renderChannels(juce::AudioBuffer<float> &dest, int startSample,
                      int numSamples);
  void processMidiMessage(const juce::MidiMessage &msg);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SF2Source)
};
