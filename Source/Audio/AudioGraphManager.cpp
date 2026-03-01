#include "Audio/AudioGraphManager.h"

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

  // Create SF2 Synth node
  auto synthProcessor = std::make_unique<SF2Source>(&mixerController);
  sf2SynthNode = mainGraph.addNode(std::move(synthProcessor));

  // Connect MIDI Input -> SF2 Synth
  mainGraph.addConnection(
      {{midiInputNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
       {sf2SynthNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});

  // Connect SF2 Synth Audio Output -> Graph Audio Output
  for (int channel = 0; channel < 2; ++channel) {
    mainGraph.addConnection(
        {{sf2SynthNode->nodeID, channel}, {audioOutputNode->nodeID, channel}});
  }

  DBG("AudioGraphManager: Graph initialized. Output channels: "
      << mainGraph.getTotalNumOutputChannels());
}

bool AudioGraphManager::loadVstPlugin(const juce::String &pluginPath,
                                      int instrumentChannel) {
  // Simplified VST loading for now. Real implementation needs proper async
  // loading and channel routing.
  juce::OwnedArray<juce::PluginDescription> typesFound;
  juce::VST3PluginFormat vst3Format;

  juce::PluginDescription desc;
  desc.fileOrIdentifier = pluginPath;
  desc.pluginFormatName = vst3Format.getName();

  juce::String errorMessage;
  if (auto instance = formatManager.createPluginInstance(
          desc, getSampleRate(), getBlockSize(), errorMessage)) {
    // Add to graph and connect (this requires a more complex routing setup in a
    // real app)
    auto node = mainGraph.addNode(std::move(instance));

    // Example: Route MIDI to this VST, and audio out to main out
    mainGraph.addConnection(
        {{midiInputNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
         {node->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});

    for (int channel = 0; channel < 2; ++channel) {
      mainGraph.addConnection(
          {{node->nodeID, channel}, {audioOutputNode->nodeID, channel}});
    }

    return true;
  }

  return false;
}

void AudioGraphManager::setSoundFont(const juce::File &sf2File) {
  if (sf2SynthNode != nullptr) {
    if (auto *synth = dynamic_cast<SF2Source *>(sf2SynthNode->getProcessor()))
      synth->loadSoundFont(sf2File);
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
  if (sf2SynthNode != nullptr) {
    if (auto *synth = sf2SynthNode->getProcessor()) {
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

  // Workaround: Direct MIDI injection
  // Instead of relying on the graph's IO nodes (which might be misconfigured),
  // we directly process the synth node if it exists, and then mix its output.
  if (sf2SynthNode != nullptr) {
    if (auto *synth = sf2SynthNode->getProcessor()) {
      synth->processBlock(buffer, midiBuffer);
    }
  } else {
    // Fallback to graph processing just in case
    processBlock(buffer, midiBuffer);
  }

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

void AudioGraphManager::getStateInformation(juce::MemoryBlock &destData) {}
void AudioGraphManager::setStateInformation(const void *data, int sizeInBytes) {
}
