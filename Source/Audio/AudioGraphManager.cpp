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
  tsf *newSynth = tsf_load_filename(path.toUTF8());
  if (newSynth != nullptr) {
    sharedSoundFonts[path] = newSynth;
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

  mainGraph.addConnection(
      {{midiInputNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
       {globalSynthNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});

  for (int channel = 0; channel < 2; ++channel) {
    mainGraph.addConnection({{globalSynthNode->nodeID, channel},
                             {audioOutputNode->nodeID, channel}});
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
      mixerController, static_cast<OutputDestination>(slotIndex)));
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
  if (globalSynthNode != nullptr) {
    mainGraph.addConnection(
        {{midiInputNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
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
