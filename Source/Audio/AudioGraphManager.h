#pragma once

#include "Core/Instruments/SF2Source.h"
#include "Core/MidiPlayer.h"
#include "Core/Routing/MixerController.h"
#include <JuceHeader.h>
#include <array>
#include <map>
#include <memory>

class AudioGraphManager : public juce::AudioProcessor {
public:
  AudioGraphManager(MixerController &mc);
  ~AudioGraphManager() override;

  //==============================================================================
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

  //==============================================================================
  juce::AudioProcessorEditor *createEditor() override { return nullptr; }
  bool hasEditor() const override { return false; }

  const juce::String getName() const override { return "Audio Graph Manager"; }
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
  // Graph Management
  void initialiseGraph();
  void rebuildGraphRouting();

  bool loadVstiPlugin(int slotIndex, const juce::String &pluginPath);
  void removeVstiPlugin(int slotIndex);

  bool loadVstFxPlugin(InstrumentGroup group, int slotIndex,
                       const juce::String &pluginPath);
  void removeVstFxPlugin(InstrumentGroup group, int slotIndex);

  void setSoundFont(const juce::File &sf2File);
  void setTrackSoundFont(InstrumentGroup group, const juce::File &sf2File);
  void resetSynthesizers();

  // Playback
  void getNextAudioBlock(juce::AudioBuffer<float> &buffer, int numSamples,
                         double sampleRate, MidiPlayer &player);

private:
  using Node = juce::AudioProcessorGraph::Node;

  juce::AudioProcessorGraph mainGraph;
  juce::AudioPluginFormatManager formatManager;

  Node::Ptr audioInputNode;
  Node::Ptr audioOutputNode;
  Node::Ptr midiInputNode;

  // Our internal SoundFont Synths (one per active group for independent FX
  // processing)
  juce::File currentSoundFont;
  std::map<InstrumentGroup, Node::Ptr> synthNodes;

  std::map<int, Node::Ptr> vstiNodes;
  std::map<int, Node::Ptr> vstiFilterNodes;
  std::map<InstrumentGroup, std::array<Node::Ptr, 4>> vstFxNodes;

  MixerController &mixerController;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioGraphManager)
};
