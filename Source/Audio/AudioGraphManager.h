#pragma once

#include "Core/Instruments/SF2Source.h"
#include "Core/MidiPlayer.h"
#include "Core/Plugins/PluginHost.h"
#include "Core/Routing/MixerController.h"
#include <JuceHeader.h>
#include <array>
#include <map>
#include <memory>

struct tsf;

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
  bool loadVstiPlugin(int slotIndex, const juce::PluginDescription &desc);
  void removeVstiPlugin(int slotIndex);
  void openVstiPluginEditor(int slotIndex);

  bool loadVstFxPlugin(InstrumentGroup group, int slotIndex,
                       const juce::String &pluginPath);
  void removeVstFxPlugin(InstrumentGroup group, int slotIndex);

  void setSoundFont(const juce::File &sf2File);
  void resetSynthesizers();

  void clearSharedSoundFonts();
  tsf *getSharedSoundFont(const juce::String &path);

  // Playback
  void getNextAudioBlock(juce::AudioBuffer<float> &buffer, int numSamples,
                         double sampleRate, MidiPlayer &player);

  PluginHost &getPluginHost() { return pluginHost; }

private:
  using Node = juce::AudioProcessorGraph::Node;

  juce::AudioProcessorGraph mainGraph;
  juce::AudioPluginFormatManager formatManager;

  Node::Ptr audioInputNode;
  Node::Ptr audioOutputNode;
  Node::Ptr midiInputNode;

  // Global SoundFont Synth
  juce::File currentSoundFont;
  Node::Ptr globalSynthNode;

  std::map<int, Node::Ptr> vstiNodes;
  std::map<int, Node::Ptr> vstiFilterNodes;
  std::map<int, juce::Component::SafePointer<juce::DocumentWindow>> vstiWindows;

  std::map<juce::String, tsf *> sharedSoundFonts;

  MixerController &mixerController;
  PluginHost pluginHost;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioGraphManager)
};
