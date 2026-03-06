#include "Audio/AudioGraphManager.h"
#include "Audio/tsf.h"
#include "Core/MidiHelper.h"
#include <juce_audio_processors/juce_audio_processors.h>
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
      // Always allow MetaEvents (Text, Lyric, Tempo, etc.) and SysEx
      if (msg.isMetaEvent() || msg.isSysEx() || ch < 1 || ch > 16) {
        filtered.addEvent(msg, metadata.samplePosition);
        continue;
      }

      if (msg.isProgramChange()) {
        channelPrograms[ch - 1] = msg.getProgramChangeNumber();
      }

      bool isDrum = (ch == 10); // MIDI channel 10 = drum channel
      InstrumentGroup group;

      if (isDrum) {
        if (msg.isNoteOnOrOff()) {
          group = MidiHelper::getInstrumentDrumType(msg.getNoteNumber());
        } else {
          // Pass through CC/Pitch for drums if any drum group is routed here
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
        // Always allow NoteOff/CC/PitchWheel so synths don't hang
        bool isStoppingMessage = msg.isNoteOff() || msg.isController() ||
                                 msg.isPitchWheel() || msg.isAllNotesOff() ||
                                 msg.isAllSoundOff();

        if (isStoppingMessage || mixerController.isTrackPlaying(group)) {
          filtered.addEvent(msg, metadata.samplePosition);

          if (msg.isNoteOn()) {
            float velocity = msg.getFloatVelocity();
            vstiSimulatedPeaks[group] =
                std::max(vstiSimulatedPeaks[group], velocity);
          }
        }
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
// AuxBusProcessor — a simple summing node for Send FX
//==============================================================================
class AuxBusProcessor : public juce::AudioProcessor {
public:
  AuxBusProcessor(MixerController &mc, InstrumentGroup g,
                  const juce::String &name)
      : AudioProcessor(
            BusesProperties()
                .withInput("Input", juce::AudioChannelSet::stereo(), true)
                .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
        mixerController(mc), group(g), busName(name) {}

  ~AuxBusProcessor() override = default;
  void prepareToPlay(double, int) override {}
  void releaseResources() override {}
  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &) override {
    // Measure the input peak so the FX strip VU meter shows incoming send
    // level.
    const int numSamples = buffer.getNumSamples();
    float peakL = 0.0f;
    float peakR = 0.0f;

    if (buffer.getNumChannels() > 0) {
      peakL = buffer.getMagnitude(0, 0, numSamples);
      peakR = peakL;
    }
    if (buffer.getNumChannels() > 1) {
      peakR = buffer.getMagnitude(1, 0, numSamples);
    }

    mixerController.setTrackLevel(group, peakL, peakR);
  }
  juce::AudioProcessorEditor *createEditor() override { return nullptr; }
  bool hasEditor() const override { return false; }
  const juce::String getName() const override { return busName; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
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
  InstrumentGroup group;
  juce::String busName;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuxBusProcessor)
};

//==============================================================================
// VstiGainProcessor — measures peak, applies simple gain, and outputs Sends
//==============================================================================
class TrackProcessor : public juce::AudioProcessor {
public:
  TrackProcessor(MixerController &mc, InstrumentGroup g)
      : AudioProcessor(
            BusesProperties()
                .withInput("Input", juce::AudioChannelSet::stereo(), true)
                .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                .withOutput("FX1", juce::AudioChannelSet::stereo(), true)
                .withOutput("FX2", juce::AudioChannelSet::stereo(), true)
                .withOutput("FX3", juce::AudioChannelSet::stereo(), true)),
        mixerController(mc), group(g) {}

  ~TrackProcessor() override = default;

  void prepareToPlay(double, int) override {}
  void releaseResources() override {}

  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &) override {
    const int numSamples = buffer.getNumSamples();
    float vol = mixerController.getTrackVolume(group);
    float pan = mixerController.getTrackPan(group);
    bool isPlaying = mixerController.isTrackPlaying(group);

    if (!isPlaying)
      vol = 0.0f;

    // Apply Main Gain and Pan to channels 0-1
    const float leftGain = vol * juce::jmin(1.0f, (1.0f - pan) * 2.0f);
    const float rightGain = vol * juce::jmin(1.0f, pan * 2.0f);

    if (buffer.getNumChannels() >= 2) {
      buffer.applyGain(0, 0, numSamples, leftGain);
      buffer.applyGain(1, 0, numSamples, rightGain);

      float peakL = buffer.getMagnitude(0, 0, numSamples);
      float peakR = buffer.getMagnitude(1, 0, numSamples);
      peakLevelL.store(peakL);
      peakLevelR.store(peakR);

      // Update Mixer Controller level for VU meters
      mixerController.setTrackLevel(group, peakL, peakR);

      // Copy to Aux Send buses (FX1, FX2, FX3)
      // We use the signal AFTER gain/panning for Post-Fader sends
      for (int auxIdx = 0; auxIdx < 3; ++auxIdx) {
        float sendLevel = mixerController.getTrackAuxSendBypass(group, auxIdx)
                              ? 0.0f
                              : mixerController.getTrackAuxSend(group, auxIdx);
        int startChannel = 2 + (auxIdx * 2);

        // If the buffer doesn't have enough channels, the graph isn't providing
        // them. This usually happens if the sends aren't connected in the
        // graph.
        if (buffer.getNumChannels() >= startChannel + 2) {
          if (sendLevel > 0.001f) {
            buffer.copyFrom(startChannel, 0, buffer, 0, 0, numSamples);
            buffer.applyGain(startChannel, 0, numSamples, sendLevel);
            buffer.copyFrom(startChannel + 1, 0, buffer, 1, 0, numSamples);
            buffer.applyGain(startChannel + 1, 0, numSamples, sendLevel);
          } else {
            buffer.clear(startChannel, 0, numSamples);
            buffer.clear(startChannel + 1, 0, numSamples);
          }
        }
      }
    }
  }

  juce::AudioProcessorEditor *createEditor() override { return nullptr; }
  bool hasEditor() const override { return false; }
  const juce::String getName() const override { return "Track Processor"; }
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

  float getPeakLevelL() const { return peakLevelL.load(); }
  float getPeakLevelR() const { return peakLevelR.load(); }

private:
  std::atomic<float> peakLevelL{0.0f};
  std::atomic<float> peakLevelR{0.0f};
  MixerController &mixerController;
  InstrumentGroup group;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackProcessor)
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

  // Create TrackProcessor nodes for each slot 1-8 (0-7 indexing internally)
  for (int i = 0; i < 8; ++i) {
    auto group = static_cast<InstrumentGroup>(
        static_cast<int>(InstrumentGroup::VSTi1) + i);
    trackGainNodes[group] = mainGraph.addNode(
        std::make_unique<TrackProcessor>(mixerController, group));
  }

  // Create AuxBus processors (Summing Nodes before FX)
  const char *auxNames[] = {"Reverb Bus", "Delay Bus", "Chorus Bus"};
  InstrumentGroup auxGroupsBus[] = {InstrumentGroup::ReverbBus,
                                    InstrumentGroup::DelayBus,
                                    InstrumentGroup::ChorusBus};
  for (int i = 0; i < 3; ++i) {
    auxBusNodes[i] = mainGraph.addNode(std::make_unique<AuxBusProcessor>(
        mixerController, auxGroupsBus[i], juce::String(auxNames[i])));
  }

  // Create Aux Return Faders (TrackProcessors after FX)
  InstrumentGroup auxGroups[] = {InstrumentGroup::ReverbBus,
                                 InstrumentGroup::DelayBus,
                                 InstrumentGroup::ChorusBus};
  for (int i = 0; i < 3; ++i) {
    auxReturnGainNodes[i] = mainGraph.addNode(
        std::make_unique<TrackProcessor>(mixerController, auxGroups[i]));
  }

  // Create Master Fader
  masterBusNode = mainGraph.addNode(std::make_unique<TrackProcessor>(
      mixerController, InstrumentGroup::MasterBus));

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
    // ปิดหน้าต่างอย่างปลอดภัยโดยให้ MessageManager เป็นผู้ลบ
    setVisible(false);
    juce::MessageManager::callAsync([c = juce::Component::SafePointer<juce::Component>(this)] {
      delete c.getComponent();
    });
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
  // 1. ค้นหา PluginDescription ตัวเต็มจาก KnownPluginList ของ PluginHost ก่อน
  auto &knownList = pluginHost.getKnownPluginList();
  juce::PluginDescription desc;
  bool found = false;

  for (int i = 0; i < knownList.getNumTypes(); ++i) {
    if (auto *p = knownList.getType(i)) {
      if (p->fileOrIdentifier == pluginPath) {
        desc = *p;
        found = true;
        break;
      }
    }
  }

  if (!found) {
    desc.fileOrIdentifier = pluginPath;
    desc.pluginFormatName = "VST3";
  }

  juce::String errorMessage;
  auto instance = pluginHost.getFormatManager().createPluginInstance(
      desc, getSampleRate(), getBlockSize(), errorMessage);

  if (!instance) {
    DBG("AudioGraphManager::loadVstFxPlugin FAILED: " + errorMessage);
    return false;
  }

  removeVstFxPlugin(group, slotIndex);

  auto fxNode = mainGraph.addNode(std::move(instance));
  fxNodes[group][slotIndex] = fxNode;

  // Save state in MixerController
  mixerController.setVstPluginPath(group, slotIndex, pluginPath);
  mixerController.setVstPluginBypass(group, slotIndex, false);

  rebuildGraphRouting();
  return true;
}

void AudioGraphManager::removeVstFxPlugin(InstrumentGroup group,
                                          int slotIndex) {
  if (fxWindows[group][slotIndex] != nullptr) {
    delete fxWindows[group][slotIndex].getComponent();
  }

  if (auto node = fxNodes[group][slotIndex]) {
    mainGraph.removeNode(node.get());
    fxNodes[group][slotIndex] = nullptr;
  }

  mixerController.setVstPluginPath(group, slotIndex, "");
  rebuildGraphRouting();
}

void AudioGraphManager::openVstFxPluginEditor(InstrumentGroup group,
                                              int slotIndex) {
  auto node = fxNodes[group][slotIndex];
  if (node == nullptr)
    return;

  auto *processor = node->getProcessor();
  if (!processor)
    return;

  if (auto *editor = processor->createEditorIfNeeded()) {
    if (fxWindows[group][slotIndex] != nullptr) {
      delete fxWindows[group][slotIndex].getComponent();
    }

    auto *window = new PluginWindow(processor->getName(), editor);
    fxWindows[group][slotIndex] = window;
    window->setVisible(true);

    juce::MessageManager::callAsync([window]() {
      if (window != nullptr)
        window->toFront(true);
    });
  }
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
  graphBuffer.setSize(2, samplesPerBlock);
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
  if (graphBuffer.getNumSamples() < numSamples) { graphBuffer.setSize(2, numSamples, false, true, true); }
  juce::AudioBuffer<float> localGraphBuffer(graphBuffer.getArrayOfWritePointers(), 2, numSamples);
  localGraphBuffer.clear();

  // Process the entire graph normally!
  juce::ScopedNoDenormals noDenormals;
  mainGraph.processBlock(localGraphBuffer, midiBuffer);

  // Copy the processed stereo audio back into the actual destination buffer
  buffer.clear();
  for (int i = 0;
       i < juce::jmin(buffer.getNumChannels(), localGraphBuffer.getNumChannels());
       ++i) {
    buffer.copyFrom(i, 0, localGraphBuffer, i, 0, numSamples);
  }

  static int emptyCounter = 0;
  if (localGraphBuffer.getMagnitude(0, localGraphBuffer.getNumSamples()) < 0.0001f) {
    if (++emptyCounter % 500 == 0)
      DBG("AudioGraphManager: Buffer is silent after processing.");
  } else {
    if (emptyCounter > 0)
      DBG("AudioGraphManager: Buffer has AUDIO from Graph!");
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

  // 2. Clear old split nodes
  for (auto &pair : splitSynthNodes) {
    if (pair.second != nullptr) {
      mainGraph.removeNode(pair.second.get());
    }
  }
  splitSynthNodes.clear();

  for (auto &pair : splitFilterNodes) {
    if (pair.second != nullptr) {
      mainGraph.removeNode(pair.second.get());
    }
  }
  splitFilterNodes.clear();

  // Find which InstrumentGroups need their own SF2Source (because they have FX)
  std::vector<InstrumentGroup> splittedGroups;
  for (int i = 0; i <= (int)InstrumentGroup::PercussionDrum; ++i) {
    auto group = static_cast<InstrumentGroup>(i);
    bool hasFx = false;
    for (int slot = 0; slot < 4; ++slot) {
      if (fxNodes[group][slot] != nullptr) {
        hasFx = true;
        break;
      }
    }

    // Only split if routed to SF2 and has FX
    if (hasFx && mixerController.getTrackOutputDestination(group) ==
                     OutputDestination::SF2) {
      splittedGroups.push_back(group);

      // Create dedicated Filter & Synth for this group
      auto filter = std::make_unique<MidiFilterProcessor>(
          mixerController, OutputDestination::SF2);
      auto synth = std::make_unique<SF2Source>(&mixerController, group);

      // Initialize the specific synth with the current SoundFont
      if (currentSoundFont.existsAsFile()) {
        synth->loadSoundFont(
            currentSoundFont,
            getSharedSoundFont(currentSoundFont.getFullPathName()));
      }

      auto filterNode = mainGraph.addNode(std::move(filter));
      auto synthNode = mainGraph.addNode(std::move(synth));

      splitFilterNodes[group] = filterNode;
      splitSynthNodes[group] = synthNode;

      // Connect MIDI Input -> Group Filter -> Group Synth
      mainGraph.addConnection(
          {{midiInputNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
           {filterNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});
      mainGraph.addConnection(
          {{filterNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
           {synthNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});
    }
  }

  // Helper lambda to route an audio node chain
  auto connectAudioChain = [&](Node::Ptr srcNode, InstrumentGroup group,
                               Node::Ptr destNode) {
    if (srcNode == nullptr || destNode == nullptr)
      return;

    Node::Ptr currentNode = srcNode;

    // Route through FX slots 0..3
    for (int slot = 0; slot < 4; ++slot) {
      if (auto fxNode = fxNodes[group][slot]) {
        // Enforce max 2 channels (stereo) routing to avoid 0-in setups
        fxNode->setBypassed(mixerController.getVstPluginBypass(group, slot));
        for (int ch = 0; ch < 2; ++ch) {
          mainGraph.addConnection(
              {{currentNode->nodeID,
                ch % currentNode->getProcessor()->getTotalNumOutputChannels()},
               {fxNode->nodeID,
                ch % fxNode->getProcessor()->getTotalNumInputChannels()}});
        }
        currentNode = fxNode;
      }
    }

    // Connect last node to destination
    for (int ch = 0; ch < 2; ++ch) {
      mainGraph.addConnection(
          {{currentNode->nodeID,
            ch % currentNode->getProcessor()->getTotalNumOutputChannels()},
           {destNode->nodeID,
            ch % destNode->getProcessor()->getTotalNumInputChannels()}});
    }
  };

  // Helper lambda to route Aux Send channels to the Aux Busses
  auto connectAuxSends = [&](Node::Ptr srcNode) {
    if (srcNode == nullptr)
      return;

    // Check if the processor has enough output channels (Main + 3x Stereo Aux)
    if (srcNode->getProcessor()->getTotalNumOutputChannels() < 8)
      return;

    for (int auxIdx = 0; auxIdx < 3; ++auxIdx) {
      if (auxBusNodes[auxIdx] != nullptr) {
        int startChannel = 2 + (auxIdx * 2);
        mainGraph.addConnection({{srcNode->nodeID, startChannel},
                                 {auxBusNodes[auxIdx]->nodeID, 0}});
        mainGraph.addConnection({{srcNode->nodeID, startChannel + 1},
                                 {auxBusNodes[auxIdx]->nodeID, 1}});
      }
    }
  };

  // 3. Route Global Synth (for non-FX tracks)
  if (globalSynthNode != nullptr) {
    if (auto *sf2Source =
            dynamic_cast<SF2Source *>(globalSynthNode->getProcessor())) {
      sf2Source->setExcludedGroups(splittedGroups);
    }
    // Global Synth -> Master Bus
    connectAudioChain(globalSynthNode, InstrumentGroup::MasterBus,
                      masterBusNode);
    connectAuxSends(globalSynthNode);
  }

  // 4. Route Split Synths
  for (auto group : splittedGroups) {
    if (auto synthNode = splitSynthNodes[group]) {
      if (trackGainNodes.find(group) == trackGainNodes.end()) {
        trackGainNodes[group] = mainGraph.addNode(
            std::make_unique<TrackProcessor>(mixerController, group));
      }

      auto destNode = trackGainNodes[group];
      connectAudioChain(synthNode, group, destNode);

      // TrackProcessor -> Master Bus
      for (int ch = 0; ch < 2; ++ch) {
        mainGraph.addConnection(
            {{destNode->nodeID, ch}, {masterBusNode->nodeID, ch}});
      }
      connectAuxSends(destNode);
    }
  }

  // 5. Route VSTi Nodes
  for (auto &pair : vstiNodes) {
    int slot = pair.first;
    if (pair.second == nullptr)
      continue;

    auto vstiGroup = static_cast<InstrumentGroup>(
        static_cast<int>(InstrumentGroup::VSTi1) + slot);
    auto destNode = trackGainNodes.count(vstiGroup) ? trackGainNodes[vstiGroup]
                                                    : masterBusNode;

    connectAudioChain(pair.second, vstiGroup, destNode);

    if (destNode != masterBusNode) {
      // TrackProcessor -> Master Bus
      for (int ch = 0; ch < 2; ++ch) {
        mainGraph.addConnection(
            {{destNode->nodeID, ch}, {masterBusNode->nodeID, ch}});
      }
      connectAuxSends(destNode);
    }
  }

  // 6. Route Aux Returns
  // Aux Summing -> FX Strip -> Aux Return Fader -> Master Bus
  InstrumentGroup auxGroups[] = {InstrumentGroup::ReverbBus,
                                 InstrumentGroup::DelayBus,
                                 InstrumentGroup::ChorusBus};
  for (int i = 0; i < 3; ++i) {
    if (auto auxSumNode = auxBusNodes[i]) {
      auto auxFaderNode = auxReturnGainNodes[i];
      if (auxFaderNode != nullptr) {
        connectAudioChain(auxSumNode, auxGroups[i], auxFaderNode);

        // Aux Return Fader -> Master Bus
        for (int ch = 0; ch < 2; ++ch) {
          mainGraph.addConnection(
              {{auxFaderNode->nodeID, ch}, {masterBusNode->nodeID, ch}});
        }
      }
    }
  }

  // 7. Route Master Bus to Hardware Output
  if (masterBusNode != nullptr) {
    // Future: add Master FX strip here if needed:
    // connectAudioChain(masterBusNode, InstrumentGroup::MasterBus,
    // audioOutputNode);
    for (int ch = 0; ch < 2; ++ch) {
      mainGraph.addConnection(
          {{masterBusNode->nodeID, ch}, {audioOutputNode->nodeID, ch}});
    }
  }
}

void AudioGraphManager::getStateInformation(juce::MemoryBlock &destData) {}
void AudioGraphManager::setStateInformation(const void *data, int sizeInBytes) {
}

void AudioGraphManager::setVstiVolume(int slotIndex, float gain) {
  // Volume is now handled automatically by TrackProcessor pulling from
  // MixerController
}

float AudioGraphManager::getVstiPeak(int slotIndex) const {
  auto vstiGroup = static_cast<InstrumentGroup>(
      static_cast<int>(InstrumentGroup::VSTi1) + slotIndex);
  auto it = trackGainNodes.find(vstiGroup);
  if (it != trackGainNodes.end() && it->second != nullptr) {
    if (auto *p = dynamic_cast<TrackProcessor *>(it->second->getProcessor()))
      return juce::jmax(p->getPeakLevelL(), p->getPeakLevelR());
  }
  return 0.0f;
}
