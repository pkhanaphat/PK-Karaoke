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

class AudioGraphManager : public juce::AudioProcessor,
                          private juce::AsyncUpdater {
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

  // Safely triggers rebuildGraphRouting on the message thread, debouncing
  // multiple consecutive calls into a single rebuild.
  void triggerRebuild() { triggerAsyncUpdate(); }

  bool loadVstiPlugin(int slotIndex, const juce::String &pluginPath);
  bool loadVstiPlugin(int slotIndex, const juce::PluginDescription &desc);
  void removeVstiPlugin(int slotIndex);
  void openVstiPluginEditor(int slotIndex);

  bool loadVstFxPlugin(InstrumentGroup group, int slotIndex,
                       const juce::String &pluginPath);
  void removeVstFxPlugin(InstrumentGroup group, int slotIndex);
  void openVstFxPluginEditor(InstrumentGroup group, int slotIndex);
  void setVstFxBypass(InstrumentGroup group, int slotIndex, bool bypass);
  void setVstiBypass(int slotIndex, bool bypass);

  void setSoundFont(const juce::File &sf2File);
  void resetSynthesizers();

  void clearSharedSoundFonts();
  tsf *getSharedSoundFont(const juce::String &path);

  // Playback
  void getNextAudioBlock(juce::AudioBuffer<float> &buffer, int numSamples,
                         double sampleRate, MidiPlayer &player);

  PluginHost &getPluginHost() { return pluginHost; }

  // Returns the display name of the loaded VSTi at slot (1-8), or empty string
  // if not loaded
  juce::String getLoadedVstiName(int slotIndex) const {
    auto it = vstiNodes.find(slotIndex);
    if (it == vstiNodes.end() || it->second == nullptr)
      return {};
    if (auto *proc = it->second->getProcessor())
      return proc->getName();
    return {};
  }

  // Returns true if at least one drum InstrumentGroup (100-115) is assigned
  // to a VSTi destination (not SF2)
  bool isDrumVstiAssigned() const {
    for (int i = 100; i <= 115; ++i) {
      auto group = static_cast<InstrumentGroup>(i);
      auto dest = mixerController.getTrackOutputDestination(group);
      if (dest != OutputDestination::SF2)
        return true;
    }
    return false;
  }

  void setVstiVolume(int slotIndex, float gain);
  float getVstiPeak(int slotIndex) const;

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
  Node::Ptr globalFilterNode;

  // Aux Summing Buses (Reverb = 0, Delay = 1, Chorus = 2)
  Node::Ptr auxBusNodes[3];

  // Aux Return Faders (Post-FX)
  Node::Ptr auxReturnGainNodes[3];

  // Master Fader
  Node::Ptr masterBusNode;

  // Pan, Volume, and Aux Send processors for each track
  std::map<InstrumentGroup, Node::Ptr> trackGainNodes;

  std::map<int, Node::Ptr> vstiNodes;
  std::map<int, Node::Ptr> vstiFilterNodes;
  std::map<int, Node::Ptr> vstiSplitNodes;
  std::map<int, juce::Component::SafePointer<juce::DocumentWindow>> vstiWindows;

  std::map<juce::String, tsf *> sharedSoundFonts;

  // Track FX Nodes
  std::map<InstrumentGroup, std::array<Node::Ptr, 4>> fxNodes;
  std::map<InstrumentGroup,
           std::array<juce::Component::SafePointer<juce::DocumentWindow>, 4>>
      fxWindows;
  std::map<InstrumentGroup, Node::Ptr> splitSynthNodes;
  std::map<InstrumentGroup, Node::Ptr> splitFilterNodes;

  MixerController &mixerController;
  PluginHost pluginHost;
  juce::AudioBuffer<float> graphBuffer;

  // Required by juce::AsyncUpdater — called on message thread to debounce
  // multiple routing rebuild requests from the UI into a single call
  void handleAsyncUpdate() override;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioGraphManager)
};
