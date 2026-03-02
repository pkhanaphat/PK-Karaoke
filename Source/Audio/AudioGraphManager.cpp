#include "Audio/AudioGraphManager.h"
#include "Core/MidiHelper.h"
#include <memory>

class MidiFilterProcessor : public juce::AudioProcessor {
public:
  MidiFilterProcessor(MixerController &mc, OutputDestination dest)
      : AudioProcessor(
            BusesProperties()
                .withInput("Input", juce::AudioChannelSet::stereo(), true)
                .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
        mixerController(mc), targetDestination(dest) {}

  ~MidiFilterProcessor() override = default;

  void prepareToPlay(double, int) override {}
  void releaseResources() override {}

  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) override {
    buffer.clear(); // We only process MIDI
    juce::MidiBuffer filtered;
    for (const auto metadata : midiMessages) {
      auto msg = metadata.getMessage();
      int ch = msg.getChannel();
      if (ch >= 1 && ch <= 16) {
        if (msg.isProgramChange()) {
          channelPrograms[ch - 1] = msg.getProgramChangeNumber();
        }
        InstrumentGroup group =
            MidiHelper::getInstrumentType(channelPrograms[ch - 1]);
        if (mixerController.getTrackOutputDestination(group) ==
            targetDestination) {
          filtered.addEvent(msg, metadata.samplePosition);
        }
      } else {
        filtered.addEvent(msg, metadata.samplePosition);
      }
    }
    midiMessages.swapWith(filtered);
  }

  juce::AudioProcessorEditor *createEditor() override { return nullptr; }
  bool hasEditor() const override { return false; }
  const juce::String getName() const override { return "MIDI Filter"; }
  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return true; }
  bool isMidiEffect() const override { return true; }
  double getTailLengthSeconds() const override { return 0.0; }
  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return ""; }
  void changeProgramName(int, const juce::String &) override {}
  void getStateInformation(juce::MemoryBlock &) override {}
  void setStateInformation(const void *, int) override {}

private:
  MixerController &mixerController;
  OutputDestination targetDestination;
  int channelPrograms[16] = {0};
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiFilterProcessor)
};

AudioGraphManager::AudioGraphManager(MixerController &mc)
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      mixerController(mc) {
  formatManager.addFormat(new juce::VST3PluginFormat());
  initialiseGraph();
}

AudioGraphManager::~AudioGraphManager() { mainGraph.clear(); }

void AudioGraphManager::initialiseGraph() {
  mainGraph.clear();

  // Create IO nodes
  audioInputNode = mainGraph.addNode(
      std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
          juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode));
  audioOutputNode = mainGraph.addNode(
      std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
          juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));
  midiInputNode = mainGraph.addNode(
      std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
          juce::AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode));

  // We don't create synth nodes here anymore. They are created dynamically
  // when a soundfont is loaded and tracks are initialized.

  DBG("AudioGraphManager: Graph initialized. Output channels: "
      << mainGraph.getTotalNumOutputChannels());
}

bool AudioGraphManager::loadVstiPlugin(int slotIndex,
                                       const juce::String &pluginPath) {
  juce::PluginDescription desc;
  desc.fileOrIdentifier = pluginPath;
  desc.pluginFormatName = "VST3";

  juce::String errorMessage;
  if (auto instance = formatManager.createPluginInstance(
          desc, getSampleRate(), getBlockSize(), errorMessage)) {
    removeVstiPlugin(slotIndex);

    auto vstiNode = mainGraph.addNode(std::move(instance));
    vstiNodes[slotIndex] = vstiNode;

    auto filterNode = mainGraph.addNode(std::make_unique<MidiFilterProcessor>(
        mixerController, static_cast<OutputDestination>(slotIndex)));
    vstiFilterNodes[slotIndex] = filterNode;

    rebuildGraphRouting();
    return true;
  }

  return false;
}

void AudioGraphManager::removeVstiPlugin(int slotIndex) {
  auto it = vstiNodes.find(slotIndex);
  if (it != vstiNodes.end()) {
    mainGraph.removeNode(it->second.get());
    vstiNodes.erase(it);
  }

  auto fit = vstiFilterNodes.find(slotIndex);
  if (fit != vstiFilterNodes.end()) {
    mainGraph.removeNode(fit->second.get());
    vstiFilterNodes.erase(fit);
  }
  rebuildGraphRouting();
}

bool AudioGraphManager::loadVstFxPlugin(InstrumentGroup group, int slotIndex,
                                        const juce::String &pluginPath) {
  if (slotIndex < 0 || slotIndex >= 4)
    return false;

  juce::PluginDescription desc;
  desc.fileOrIdentifier = pluginPath;
  desc.pluginFormatName = "VST3";

  juce::String errorMessage;
  if (auto instance = formatManager.createPluginInstance(
          desc, getSampleRate(), getBlockSize(), errorMessage)) {
    removeVstFxPlugin(group, slotIndex);

    auto node = mainGraph.addNode(std::move(instance));
    vstFxNodes[group][slotIndex] = node;

    mixerController.setVstPluginPath(group, slotIndex, pluginPath);
    rebuildGraphRouting();
    return true;
  }
  return false;
}

void AudioGraphManager::removeVstFxPlugin(InstrumentGroup group,
                                          int slotIndex) {
  if (slotIndex < 0 || slotIndex >= 4)
    return;
  auto it = vstFxNodes.find(group);
  if (it != vstFxNodes.end() && it->second[slotIndex] != nullptr) {
    mainGraph.removeNode(it->second[slotIndex].get());
    it->second[slotIndex] = nullptr;
    mixerController.setVstPluginPath(group, slotIndex, "");
    rebuildGraphRouting();
  }
}

void AudioGraphManager::setSoundFont(const juce::File &sf2File) {
  currentSoundFont = sf2File;

  // If we have existing synths, update them all to the new global default
  // unless they have a custom one (this is a simple implementation,
  // in a more complex one we might want to check the MixerController's sf2Path)
  for (auto &pair : synthNodes) {
    if (auto *sf2Source =
            dynamic_cast<SF2Source *>(pair.second->getProcessor())) {
      // If the controller doesn't have a custom path for this group, use the
      // global one
      if (mixerController.getTrackSF2Path(pair.first).isEmpty()) {
        sf2Source->loadSoundFont(sf2File);
      }
    }
  }
}

void AudioGraphManager::setTrackSoundFont(InstrumentGroup group,
                                          const juce::File &sf2File) {
  auto it = synthNodes.find(group);
  if (it != synthNodes.end()) {
    if (auto *sf2Source =
            dynamic_cast<SF2Source *>(it->second->getProcessor())) {
      // Check if the path is ALREADY loaded in the engine
      if (sf2Source->getLoadedPath() == sf2File.getFullPathName())
        return;

      sf2Source->loadSoundFont(sf2File);
      mixerController.setTrackSF2Path(group, sf2File.getFullPathName());
    }
  } else {
    // Create a new node for this group if it doesn't exist
    auto synthProcessor = std::make_unique<SF2Source>(&mixerController, group);
    synthProcessor->loadSoundFont(sf2File);
    auto node = mainGraph.addNode(std::move(synthProcessor));
    synthNodes[group] = node;
    mixerController.setTrackSF2Path(group, sf2File.getFullPathName());

    // Route MIDI In -> Synth
    mainGraph.addConnection(
        {{midiInputNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
         {node->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});

    // Route Synth -> Audio Out
    for (int channel = 0; channel < 2; ++channel) {
      mainGraph.addConnection(
          {{node->nodeID, channel}, {audioOutputNode->nodeID, channel}});
    }
  }
}

void AudioGraphManager::resetSynthesizers() {
  juce::MidiBuffer resetBuffer;
  for (int channel = 1; channel <= 16; ++channel) {
    resetBuffer.addEvent(juce::MidiMessage::allSoundOff(channel), 0);
    resetBuffer.addEvent(juce::MidiMessage::controllerEvent(channel, 121, 0),
                         0); // Reset All Controllers
    resetBuffer.addEvent(juce::MidiMessage::allNotesOff(channel), 0);
    resetBuffer.addEvent(juce::MidiMessage::controllerEvent(channel, 0, 0),
                         0); // Bank Select MSB
    resetBuffer.addEvent(juce::MidiMessage::controllerEvent(channel, 32, 0),
                         0); // Bank Select LSB
    resetBuffer.addEvent(juce::MidiMessage::programChange(channel, 0), 0);
  }

  // Workaround: Direct MIDI injection for the reset buffer
  for (auto &pair : synthNodes) {
    if (auto *synth = pair.second->getProcessor()) {
      juce::AudioBuffer<float> dummyBuffer(2, 64);
      dummyBuffer.clear();
      synth->processBlock(dummyBuffer, resetBuffer);
    }
  }
}

void AudioGraphManager::prepareToPlay(double sampleRate, int samplesPerBlock) {
  mainGraph.setPlayConfigDetails(getTotalNumInputChannels(),
                                 getTotalNumOutputChannels(), sampleRate,
                                 samplesPerBlock);
  mainGraph.prepareToPlay(sampleRate, samplesPerBlock);
}

void AudioGraphManager::releaseResources() { mainGraph.releaseResources(); }

void AudioGraphManager::processBlock(juce::AudioBuffer<float> &buffer,
                                     juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  mainGraph.processBlock(buffer, midiMessages);
}

void AudioGraphManager::getNextAudioBlock(juce::AudioBuffer<float> &buffer,
                                          int numSamples, double sampleRate,
                                          MidiPlayer &player) {
  // Retrieve MIDI events for this block from the player
  juce::MidiBuffer midiBuffer;
  player.getNextAudioBlock(midiBuffer, numSamples, sampleRate);

  // Process the graph (which includes our synth and any VSTs)
  buffer.clear();

  // Process the entire graph normally!
  processBlock(buffer, midiBuffer);

  static int emptyCounter = 0;
  if (buffer.getMagnitude(0, buffer.getNumSamples()) < 0.0001f) {
    if (++emptyCounter % 500 == 0)
      DBG("AudioGraphManager: Buffer is silent after processing.");
  } else {
    if (emptyCounter > 0)
      DBG("AudioGraphManager: Buffer has AUDIO!");
    emptyCounter = 0;
  }
}

void AudioGraphManager::rebuildGraphRouting() {
  for (const auto &connection : mainGraph.getConnections()) {
    mainGraph.removeConnection(connection);
  }

  if (midiInputNode == nullptr || audioOutputNode == nullptr)
    return;

  // 1. Connect MIDI Input
  for (auto &pair : synthNodes) {
    if (pair.second != nullptr) {
      mainGraph.addConnection(
          {{midiInputNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
           {pair.second->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});
    }
  }
  for (auto &pair : vstiNodes) {
    int slotIndex = pair.first;
    auto vstiNode = pair.second;
    auto filterNode = vstiFilterNodes[slotIndex];
    if (vstiNode != nullptr && filterNode != nullptr) {
      // Connect MIDI Input -> Filter
      mainGraph.addConnection(
          {{midiInputNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
           {filterNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});
      // Connect Filter -> VSTI
      mainGraph.addConnection(
          {{filterNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
           {vstiNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});
    }
  }

  // 2. Route each active channel
  auto routeGroupChain = [&](InstrumentGroup group, Node::Ptr source) {
    if (source == nullptr)
      return;
    Node::Ptr lastNode = source;

    if (vstFxNodes.count(group)) {
      for (int i = 0; i < 4; ++i) {
        auto fxNode = vstFxNodes[group][i];
        if (fxNode != nullptr &&
            !mixerController.getVstPluginBypass(group, i)) {
          int sourceChans =
              lastNode->getProcessor()->getTotalNumOutputChannels();
          int destChans = fxNode->getProcessor()->getTotalNumInputChannels();
          int numChansToConnect = juce::jmin(sourceChans, destChans, 2);

          for (int ch = 0; ch < numChansToConnect; ++ch) {
            mainGraph.addConnection(
                {{lastNode->nodeID, ch}, {fxNode->nodeID, ch}});
          }
          lastNode = fxNode;
        }
      }
    }

    int outChans = lastNode->getProcessor()->getTotalNumOutputChannels();
    for (int ch = 0; ch < juce::jmin(outChans, 2); ++ch) {
      mainGraph.addConnection(
          {{lastNode->nodeID, ch}, {audioOutputNode->nodeID, ch}});
    }
  };

  for (auto &pair : synthNodes)
    routeGroupChain(pair.first, pair.second);

  for (auto &pair : vstiNodes) {
    if (pair.second != nullptr) {
      int outChans = pair.second->getProcessor()->getTotalNumOutputChannels();
      for (int ch = 0; ch < juce::jmin(outChans, 2); ++ch) {
        mainGraph.addConnection(
            {{pair.second->nodeID, ch}, {audioOutputNode->nodeID, ch}});
      }
    }
  }
}

void AudioGraphManager::getStateInformation(juce::MemoryBlock &destData) {}
void AudioGraphManager::setStateInformation(const void *data, int sizeInBytes) {
}
