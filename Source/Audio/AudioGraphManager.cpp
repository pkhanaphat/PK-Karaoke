#include "Audio/AudioGraphManager.h"
#include "Audio/tsf.h"
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
    buffer.clear();
    juce::MidiBuffer filtered;
    for (const auto metadata : midiMessages) {
      auto msg = metadata.getMessage();
      int ch = msg.getChannel();
      if (ch >= 1 && ch <= 16) {
        if (msg.isProgramChange()) {
          channelPrograms[ch - 1] = msg.getProgramChangeNumber();
        }

        bool isDrum = (ch == 10); // MIDI channel 10 = drum channel

        InstrumentGroup group;
        if (isDrum) {
          // For drums: route based on note number (for note-on/off),
          // or pass through if any drum group is routed to this destination
          if (msg.isNoteOnOrOff()) {
            group = MidiHelper::getInstrumentDrumType(msg.getNoteNumber());
          } else {
            // Non-note drum messages (CC, pitch, etc.) — pass through to
            // this dest if any drum group is assigned to it
            bool anyDrumRouted = false;
            for (int n = 0; n < 128; ++n) {
              InstrumentGroup g = MidiHelper::getInstrumentDrumType(n);
              if (mixerController.getTrackOutputDestination(g) ==
                  targetDestination) {
                anyDrumRouted = true;
                break;
              }
            }
            if (anyDrumRouted)
              filtered.addEvent(msg, metadata.samplePosition);
            continue;
          }
        } else {
          group = MidiHelper::getInstrumentType(channelPrograms[ch - 1]);
        }

        if (mixerController.getTrackOutputDestination(group) ==
            targetDestination) {

          if (mixerController.isTrackPlaying(group)) {
            filtered.addEvent(msg, metadata.samplePosition);

            // Simulating VU Meter for VSTi output since we don't have
            // per-instrument audio streams
            if (msg.isNoteOn()) {
              float velocity = msg.getFloatVelocity();
              vstiSimulatedPeaks[group] =
                  std::max(vstiSimulatedPeaks[group], velocity);
            }
          }
        }
      } else {
        filtered.addEvent(msg, metadata.samplePosition);
      }
    }
    midiMessages.swapWith(filtered);

    // Apply artificial VU decay and push to UI for all VSTi-routed instruments
    for (int i = 0; i <= (int)InstrumentGroup::VocalBus; ++i) {
      InstrumentGroup group = static_cast<InstrumentGroup>(i);
      if (mixerController.getTrackOutputDestination(group) ==
          targetDestination) {
        float peak = vstiSimulatedPeaks[group];
        if (peak > 0.001f) {
          mixerController.setTrackLevel(group, peak, peak);
          vstiSimulatedPeaks[group] *= 0.85f; // Decay by 15% per block
        } else {
          vstiSimulatedPeaks[group] = 0.0f;
          mixerController.setTrackLevel(group, 0.0f, 0.0f);
        }
      }
    }
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
  std::map<InstrumentGroup, float> vstiSimulatedPeaks;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiFilterProcessor)
};

//==============================================================================
// VstiGainProcessor — measures peak and applies simple gain for VSTi slots
//==============================================================================
class VstiGainProcessor : public juce::AudioProcessor {
public:
  VstiGainProcessor()
      : AudioProcessor(
            BusesProperties()
                .withInput("Input", juce::AudioChannelSet::stereo(), true)
                .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}

  ~VstiGainProcessor() override = default;

  void prepareToPlay(double, int) override {}
  void releaseResources() override {}

  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &) override {
    const float g = gain.load();
    if (std::abs(g - 1.0f) > 0.0001f)
      buffer.applyGain(g);

    float currentPeak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
      currentPeak = juce::jmax(
          currentPeak, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
    }
    peakLevel.store(currentPeak);
  }

  juce::AudioProcessorEditor *createEditor() override { return nullptr; }
  bool hasEditor() const override { return false; }
  const juce::String getName() const override { return "VSTi Gain"; }
  bool acceptsMidi() const override { return false; }
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

  void setGain(float g) { gain.store(g); }
  float getPeakLevel() const { return peakLevel.load(); }

private:
  std::atomic<float> gain{1.0f};
  std::atomic<float> peakLevel{0.0f};
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VstiGainProcessor)
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

AudioGraphManager::~AudioGraphManager() {
  mainGraph.clear();
  clearSharedSoundFonts();
}

void AudioGraphManager::clearSharedSoundFonts() {
  for (auto &pair : sharedSoundFonts) {
    if (pair.second != nullptr) {
      tsf_close(pair.second);
    }
  }
  sharedSoundFonts.clear();
}

tsf *AudioGraphManager::getSharedSoundFont(const juce::String &path) {
  if (path.isEmpty() || !juce::File(path).existsAsFile()) {
    return nullptr;
  }
  auto it = sharedSoundFonts.find(path);
  if (it != sharedSoundFonts.end()) {
    return it->second;
  }

  // Load new and cache
  // Use JUCE to read file data so we get proper Unicode path support
  // (tsf_load_filename uses fopen which doesn't handle Unicode on Windows)
  juce::MemoryBlock fileData;
  if (!juce::File(path).loadFileAsData(fileData)) {
    juce::File("C:\\temp\\crash.log")
        .appendText("getSharedSoundFont: Failed to read file: " + path + "\n");
    return nullptr;
  }
  tsf *newSynth = tsf_load_memory(fileData.getData(), (int)fileData.getSize());
  if (newSynth != nullptr) {
    sharedSoundFonts[path] = newSynth;
  } else {
    juce::File("C:\\temp\\crash.log")
        .appendText(
            "getSharedSoundFont: tsf_load_memory returned nullptr for: " +
            path + "\n");
  }
  return newSynth;
}

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

  // Create the single global synth node
  auto synthProcessor =
      std::make_unique<SF2Source>(&mixerController, std::nullopt);
  globalSynthNode = mainGraph.addNode(std::move(synthProcessor));

  // Create the filter for global synth (only allows SF2-routed channels)
  auto globalFilterProcessor = std::make_unique<MidiFilterProcessor>(
      mixerController, OutputDestination::SF2);
  globalFilterNode = mainGraph.addNode(std::move(globalFilterProcessor));

  mainGraph.addConnection(
      {{midiInputNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
       {globalFilterNode->nodeID,
        juce::AudioProcessorGraph::midiChannelIndex}});

  mainGraph.addConnection(
      {{globalFilterNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
       {globalSynthNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});

  for (int channel = 0; channel < 2; ++channel) {
    mainGraph.addConnection({{globalSynthNode->nodeID, channel},
                             {audioOutputNode->nodeID, channel}});
  }

  // Create VstiGainProcessor nodes for each slot 1-8 (0-7 indexing internally)
  for (int i = 0; i < 8; ++i) {
    vstiGainNodes[i] = mainGraph.addNode(std::make_unique<VstiGainProcessor>());
  }

  DBG("AudioGraphManager: Graph initialized. Output channels: "
      << mainGraph.getTotalNumOutputChannels());
}

bool AudioGraphManager::loadVstiPlugin(int slotIndex,
                                       const juce::String &pluginPath) {
  // 1. ค้นหา PluginDescription ตัวเต็มจาก KnownPluginList ของ PluginHost ก่อน
  auto &knownList = pluginHost.getKnownPluginList();
  for (int i = 0; i < knownList.getNumTypes(); ++i) {
    if (auto *p = knownList.getType(i)) {
      if (p->fileOrIdentifier == pluginPath) {
        return loadVstiPlugin(slotIndex, *p);
      }
    }
  }

  // 2. ถ้าหาไม่เจอ ให้ลองสร้างแบบลอยๆ เผื่อกรณีที่เป็นไฟล์ VST3 ตรงๆ
  juce::PluginDescription desc;
  desc.fileOrIdentifier = pluginPath;
  desc.pluginFormatName = "VST3";
  return loadVstiPlugin(slotIndex, desc);
}

bool AudioGraphManager::loadVstiPlugin(int slotIndex,
                                       const juce::PluginDescription &desc) {
  juce::String errorMessage;
  auto instance = pluginHost.getFormatManager().createPluginInstance(
      desc, getSampleRate(), getBlockSize(), errorMessage);

  if (!instance) {
    DBG("AudioGraphManager::loadVstiPlugin FAILED: " + errorMessage);
    return false;
  }

  removeVstiPlugin(slotIndex);

  auto vstiNode = mainGraph.addNode(std::move(instance));
  vstiNodes[slotIndex] = vstiNode;

  auto filterNode = mainGraph.addNode(std::make_unique<MidiFilterProcessor>(
      mixerController, static_cast<OutputDestination>(slotIndex + 1)));
  vstiFilterNodes[slotIndex] = filterNode;

  rebuildGraphRouting();
  return true;
}

void AudioGraphManager::removeVstiPlugin(int slotIndex) {
  // 1. ปิดหน้าต่าง UI ก่อนทำลายตัว Plugin เสมอ! ป้องกัน UI แครชเมื่อหา Processor ไม่เจอ
  if (vstiWindows[slotIndex] != nullptr) {
    delete vstiWindows[slotIndex].getComponent();
  }

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

// คลาสเสริมสำหรับแสดง UI ของ Plugin ที่จะลบตัวเองทิ้งเมื่อกดปุ่ม X
class PluginWindow : public juce::DocumentWindow {
public:
  PluginWindow(juce::String name, juce::AudioProcessorEditor *editor)
      : DocumentWindow(name, juce::Colours::darkgrey,
                       juce::DocumentWindow::closeButton, false) {
    setContentOwned(editor, true);
    setResizable(true, false);
    centreWithSize(editor->getWidth() > 0 ? editor->getWidth() : 600,
                   editor->getHeight() > 0 ? editor->getHeight() : 400);
    setUsingNativeTitleBar(false);

    // ไม่ผูกหน้าต่างนี้เข้ากับ MainWindow เพื่อให้มันหลุดจากการจัดลำดับของ OS
    // ช่วยให้มันสามารถลอยอยู่หน้าสุดเหนือหน้าต่างท็อปอื่นๆ (เช่น Settings) ได้จริงๆ
    addToDesktop(getDesktopWindowStyleFlags());

    // เมื่อโปรแกรมหลักไม่ได้ตั้ง AlwaysOnTop แล้ว การตั้งค่าให้หน้าต่างย่อยเป็น AlwaysOnTop
    // จะทำให้มันอยู่เหนือหน้าหลักและไม่โดนบังตอนคลิกหน้าต่างหลักอย่างสมบูรณ์
    setAlwaysOnTop(true);
    // ดันให้เป็น AlwaysOnTop ด้วย เพื่อให้อยู่เหนือ MainWindow ที่เป็น AlwaysOnTop
  }

  void closeButtonPressed() override {
    // ปิดหน้าต่างและลบวัตถุทิ้งเพื่อให้คืน memory
    delete this;
  }
};

void AudioGraphManager::openVstiPluginEditor(int slotIndex) {
  auto it = vstiNodes.find(slotIndex);
  if (it == vstiNodes.end() || it->second == nullptr)
    return;

  auto *processor = it->second->getProcessor();
  if (!processor)
    return;

  if (auto *editor = processor->createEditorIfNeeded()) {
    // 1. ปิดหน้าต่างเก่าถ้ามีเปิดคาอยู่ก่อนสร้างหน้าต่างใหม่
    if (vstiWindows[slotIndex] != nullptr) {
      delete vstiWindows[slotIndex].getComponent();
    }

    // 2. สร้างหน้าต่างใหม่พร้อมแสดงผล
    auto *window = new PluginWindow(processor->getName(), editor);
    vstiWindows[slotIndex] = window;
    window->setVisible(true);

    // หน่วงเวลาให้ toFront ทำงานหลังจากที่ระบบประมวลผลการคลิกเมาส์เสร็จสิ้นแล้ว
    // จะช่วยป้องกันไม่ให้หน้าต่างตั้งค่าดึงตัวเองกลับไปข้างหน้า
    juce::MessageManager::callAsync([window]() {
      if (window != nullptr)
        window->toFront(true);
    });
  }
}

bool AudioGraphManager::loadVstFxPlugin(InstrumentGroup group, int slotIndex,
                                        const juce::String &pluginPath) {
  // temporarily disabled since we are using a single synth graph
  return false;
}

void AudioGraphManager::removeVstFxPlugin(InstrumentGroup group,
                                          int slotIndex) {
  // temporarily disabled since we are using a single synth graph
}

void AudioGraphManager::setSoundFont(const juce::File &sf2File) {
  juce::File("C:\\temp\\crash.log")
      .appendText(
          "AudioGraphManager Loading SF2: " + sf2File.getFullPathName() + "\n");
  currentSoundFont = sf2File;

  tsf *sharedSynth = nullptr;
  if (sf2File.existsAsFile()) {
    sharedSynth = getSharedSoundFont(sf2File.getFullPathName());
  }

  if (globalSynthNode != nullptr) {
    if (auto *sf2Source =
            dynamic_cast<SF2Source *>(globalSynthNode->getProcessor())) {
      sf2Source->loadSoundFont(sf2File, sharedSynth);
    }
  }

  // Ensure routing is maintained after a synth is constructed or updated.
  rebuildGraphRouting();
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
  if (globalSynthNode != nullptr) {
    if (auto *synth = globalSynthNode->getProcessor()) {
      juce::AudioBuffer<float> dummyBuffer(2, 64);
      dummyBuffer.clear();
      synth->processBlock(dummyBuffer, resetBuffer);
    }
  }
}

void AudioGraphManager::prepareToPlay(double sampleRate, int samplesPerBlock) {
  juce::File("C:\\temp\\crash.log")
      .appendText("AudioGraphManager::prepareToPlay Called.\n");
  if (globalSynthNode != nullptr && !currentSoundFont.existsAsFile()) {
    auto defaultPath = mixerController.getGlobalSF2Path();
    juce::File("C:\\temp\\crash.log")
        .appendText("prepareToPlay Default SF2 Path: " + defaultPath + "\n");
    if (defaultPath.isNotEmpty()) {
      setSoundFont(juce::File(defaultPath));
    }
  }

  // The AudioProcessorPlayer gives KaraokeEngine a stereo buffer,
  // so the graph must also be prepared with 2 inputs and 2 outputs
  // otherwise it will silently reject the buffer and drop all audio.
  mainGraph.setPlayConfigDetails(2, 2, sampleRate, samplesPerBlock);
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

  // The output buffer might have 0 inputs if the OS audio device has no mic.
  // But AudioProcessorGraph is prepared with 2 inputs and 2 outputs.
  // We MUST process into a guaranteed 2-in/2-out buffer to avoid silent
  // rejection.
  juce::AudioBuffer<float> graphBuffer(2, numSamples);
  graphBuffer.clear();

  // Process the entire graph normally!
  juce::ScopedNoDenormals noDenormals;
  mainGraph.processBlock(graphBuffer, midiBuffer);

  // Copy the processed stereo audio back into the actual destination buffer
  buffer.clear();
  for (int i = 0;
       i < juce::jmin(buffer.getNumChannels(), graphBuffer.getNumChannels());
       ++i) {
    buffer.copyFrom(i, 0, graphBuffer, i, 0, numSamples);
  }

  static int emptyCounter = 0;
  if (graphBuffer.getMagnitude(0, graphBuffer.getNumSamples()) < 0.0001f) {
    if (++emptyCounter % 500 == 0)
      juce::File("C:\\temp\\crash.log")
          .appendText(
              "AudioGraphManager: Buffer is silent after processing.\n");
  } else {
    if (emptyCounter > 0)
      juce::File("C:\\temp\\crash.log")
          .appendText("AudioGraphManager: Buffer has AUDIO from Graph!\n");
    emptyCounter = 0;
  }
}

void AudioGraphManager::rebuildGraphRouting() {
  auto connections = mainGraph.getConnections();
  for (const auto &connection : connections) {
    mainGraph.removeConnection(connection);
  }

  if (midiInputNode == nullptr || audioOutputNode == nullptr)
    return;

  // Push latest custom SF2 routing down to the SF2Source engine
  if (globalSynthNode != nullptr) {
    if (auto *sf2Source =
            dynamic_cast<SF2Source *>(globalSynthNode->getProcessor())) {
      std::map<InstrumentGroup, juce::String> currentPaths;
      for (int i = 0; i <= (int)InstrumentGroup::Percussive; ++i) {
        InstrumentGroup group = static_cast<InstrumentGroup>(i);
        juce::String path = mixerController.getTrackSF2Path(group);
        if (path.isNotEmpty() && mixerController.getTrackOutputDestination(
                                     group) == OutputDestination::SF2) {
          currentPaths[group] = path;
        }
      }
      for (int i = 100; i <= (int)InstrumentGroup::PercussionDrum; ++i) {
        InstrumentGroup group = static_cast<InstrumentGroup>(i);
        juce::String path = mixerController.getTrackSF2Path(group);
        if (path.isNotEmpty() && mixerController.getTrackOutputDestination(
                                     group) == OutputDestination::SF2) {
          currentPaths[group] = path;
        }
      }
      sf2Source->updateCustomSF2Routing(currentPaths,
                                        [this](const juce::String &path) {
                                          return this->getSharedSoundFont(path);
                                        });
    }
  }

  // 1. Connect MIDI Input
  if (globalSynthNode != nullptr && globalFilterNode != nullptr) {
    mainGraph.addConnection(
        {{midiInputNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
         {globalFilterNode->nodeID,
          juce::AudioProcessorGraph::midiChannelIndex}});

    mainGraph.addConnection({{globalFilterNode->nodeID,
                              juce::AudioProcessorGraph::midiChannelIndex},
                             {globalSynthNode->nodeID,
                              juce::AudioProcessorGraph::midiChannelIndex}});
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

  // 2. Route active channel (Simplified for Global Synth)
  if (globalSynthNode != nullptr) {
    int outChans = globalSynthNode->getProcessor()->getTotalNumOutputChannels();
    for (int ch = 0; ch < juce::jmin(outChans, 2); ++ch) {
      mainGraph.addConnection(
          {{globalSynthNode->nodeID, ch}, {audioOutputNode->nodeID, ch}});
    }
  }

  for (auto &pair : vstiNodes) {
    int slot = pair.first;
    if (pair.second == nullptr)
      continue;

    int outChans = pair.second->getProcessor()->getTotalNumOutputChannels();

    // 3. Connect VSTi Node -> VstiGainNode -> AudioOutputNode
    auto &gainNode = vstiGainNodes[slot];
    if (gainNode != nullptr) {
      for (int ch = 0; ch < juce::jmin(outChans, 2); ++ch) {
        mainGraph.addConnection(
            {{pair.second->nodeID, ch}, {gainNode->nodeID, ch}});
        mainGraph.addConnection(
            {{gainNode->nodeID, ch}, {audioOutputNode->nodeID, ch}});
      }
    } else {
      // Fallback if gain node is missing for some reason
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

void AudioGraphManager::setVstiVolume(int slotIndex, float gain) {
  if (slotIndex >= 0 && slotIndex < 8 && vstiGainNodes[slotIndex] != nullptr)
    if (auto *p = dynamic_cast<VstiGainProcessor *>(
            vstiGainNodes[slotIndex]->getProcessor()))
      p->setGain(gain);
}

float AudioGraphManager::getVstiPeak(int slotIndex) const {
  if (slotIndex >= 0 && slotIndex < 8 && vstiGainNodes[slotIndex] != nullptr)
    if (auto *p = dynamic_cast<VstiGainProcessor *>(
            vstiGainNodes[slotIndex]->getProcessor()))
      return p->getPeakLevel();
  return 0.0f;
}
