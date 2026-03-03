#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <map>
#include <vector>

// InstrumentGroup เนเธฅเธฐ TrackState
// เธขเนเธฒเธขเธกเธฒเธญเธขเธนเนเธ—เธตเน
// Core/Routing
// เน€เธเธทเนเธญเนเธซเนเธ—เธฑเนเธ
// SF2Source, AudioGraphManager, MixerComponent
// เนเธเนเธฃเนเธงเธกเธเธฑเธเนเธ”เน

enum class InstrumentGroup {
  // Melodic (CH1-28)
  Piano = 0,
  ChromaticPercussion,
  ThaiRanad,
  ThaiPongLang,
  Organ,
  Accordion,
  ThaiKaen,
  AcousticGuitarNylon,
  AcousticGuitarSteel,
  ElectricGuitarJazz,
  ElectricGuitarClean,
  OverdrivenGuitar,
  DistortionGuitar,
  Bass,
  Strings,
  Ensemble,
  Brass,
  Saxophone,
  Reed,
  Pipe,
  ThaiKlui,
  ThaiWote,
  SynthLead,
  SynthPad,
  SynthEffects,
  Ethnic,
  ThaiPoengMang,
  Percussive,

  // Drums (CH29-44)
  Kick = 100,
  Snare,
  HandClap,
  HiTom,
  MidTom,
  LowTom,
  HiHat,
  CrashCymbal,
  RideCymbal,
  Tambourine,
  Cowbell,
  Bongo,
  Conga,
  Timbale,
  ThaiChing,
  PercussionDrum,

  // Vocal
  VocalBus = 149,

  // FX & Master Bus
  ReverbBus = 200,
  DelayBus,
  ChorusBus,
  MasterBus
};

struct VstPluginState {
  juce::String path;
  bool isBypass{false};
};

enum class OutputDestination {
  SF2 = 0,
  VSTi1,
  VSTi2,
  VSTi3,
  VSTi4,
  VSTi5,
  VSTi6,
  VSTi7,
  VSTi8
};

struct TrackState {
  std::atomic<float> volume{1.0f}; // Linear fader gain
  std::atomic<float> pan{0.5f};    // 0 = Left, 0.5 = Center, 1 = Right
  std::atomic<float> gain{1.0f};   // Input trim/gain
  std::atomic<float> auxSends[3];  // 0: Reverb, 1: Delay, 2: Chorus
  std::atomic<int> transpose{0};   // Transpose in semitones
  std::atomic<OutputDestination> outputDest{OutputDestination::SF2};

  juce::String sf2Path;      // Custom SoundFont for this track
  VstPluginState inserts[4]; // 4 Insert Slots

  std::atomic<bool> isMuted{false};
  std::atomic<bool> isSolo{false};

  std::atomic<float> currentPeakLeft{0.0f};  // Current peak level for L meter
  std::atomic<float> currentPeakRight{0.0f}; // Current peak level for R meter

  TrackState() {
    for (int i = 0; i < 3; ++i)
      auxSends[i].store(0.0f);
  }

  TrackState(const TrackState &other) {
    volume.store(other.volume.load());
    pan.store(other.pan.load());
    gain.store(other.gain.load());
    for (int i = 0; i < 3; ++i)
      auxSends[i].store(other.auxSends[i].load());
    transpose.store(other.transpose.load());
    outputDest.store(other.outputDest.load());
    isMuted.store(other.isMuted.load());
    isSolo.store(other.isSolo.load());
    for (int i = 0; i < 4; ++i) {
      inserts[i].path = other.inserts[i].path;
      inserts[i].isBypass = other.inserts[i].isBypass;
    }
    sf2Path = other.sf2Path;
  }
};

class MixerController {
public:
  MixerController();
  ~MixerController() = default;

  void setTrackVolume(InstrumentGroup group, float linearGain);
  float getTrackVolume(InstrumentGroup group) const;

  void setTrackPan(InstrumentGroup group, float panValue);
  float getTrackPan(InstrumentGroup group) const;

  void setTrackGain(InstrumentGroup group, float linearGain);
  float getTrackGain(InstrumentGroup group) const;

  void setTrackAuxSend(InstrumentGroup group, int auxIndex, float linearGain);
  float getTrackAuxSend(InstrumentGroup group, int auxIndex) const;

  void setTrackMute(InstrumentGroup group, bool shouldMute);
  bool isTrackMuted(InstrumentGroup group) const;

  void setTrackSolo(InstrumentGroup group, bool shouldSolo);
  bool isTrackSolo(InstrumentGroup group) const;

  bool isAnySoloActive() const { return anySoloActive; }
  bool isEffectivelyMuted(InstrumentGroup group) const;

  bool isTrackPlaying(InstrumentGroup group) const;

  float getTrackLevelLeft(InstrumentGroup group) const;
  float getTrackLevelRight(InstrumentGroup group) const;
  void setTrackLevel(InstrumentGroup group, float peakL, float peakR);

  void setTrackTranspose(InstrumentGroup group, int transposeValue);
  int getTrackTranspose(InstrumentGroup group) const;

  void initializeTrack(InstrumentGroup group);

  void setVstPluginPath(InstrumentGroup group, int slotIndex,
                        const juce::String &path);
  juce::String getVstPluginPath(InstrumentGroup group, int slotIndex) const;

  void setVstPluginBypass(InstrumentGroup group, int slotIndex, bool bypass);
  bool getVstPluginBypass(InstrumentGroup group, int slotIndex) const;

  void setTrackOutputDestination(InstrumentGroup group, OutputDestination dest);
  OutputDestination getTrackOutputDestination(InstrumentGroup group) const;

  void setTrackSF2Path(InstrumentGroup group, const juce::String &path);
  juce::String getTrackSF2Path(InstrumentGroup group) const;

  // Global SF2 (ใช้เมื่อ Track ไม่ได้เลือก SF2 เอง)
  void setGlobalSF2Path(const juce::String &path) { globalSf2Path = path; }
  juce::String getGlobalSF2Path() const { return globalSf2Path; }

  // SF2 Folder Scanning
  void setSF2Directory(const juce::File &directory);
  juce::File getSF2Directory() const { return sf2Directory; }
  void updateSF2List();
  juce::StringArray getAvailableSF2Names() const;
  juce::File getSF2FileByName(const juce::String &name) const;

  // Called by AudioEngine inside processBlock
  void processBuffer(juce::AudioBuffer<float> &buffer, InstrumentGroup group);

private:
  std::map<InstrumentGroup, TrackState> tracks;
  bool anySoloActive = false;
  void updateSoloState();
  juce::CriticalSection lock;

  juce::File sf2Directory;
  juce::Array<juce::File> availableSf2Files;
  juce::String globalSf2Path;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerController)
};
