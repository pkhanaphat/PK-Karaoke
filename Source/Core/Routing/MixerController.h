#pragma once
#include <JuceHeader.h>
#include <map>

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

  // VSTi slots
  VSTi1 = 150,
  VSTi2,
  VSTi3,
  VSTi4,
  VSTi5,
  VSTi6,
  VSTi7,
  VSTi8,

  // FX & Master Bus
  ReverbBus = 200,
  DelayBus,
  ChorusBus,
  MasterBus
};

struct TrackState {
  std::atomic<float> volume{1.0f}; // Linear fader gain
  std::atomic<float> pan{0.5f};    // 0 = Left, 0.5 = Center, 1 = Right
  std::atomic<float> gain{1.0f};   // Input trim/gain
  std::atomic<float> auxSends[3];  // 0: Reverb, 1: Delay, 2: Chorus

  std::atomic<bool> isMuted{false};
  std::atomic<bool> isSolo{false};

  std::atomic<float> currentMagnitude{0.0f}; // Current peak level for meters

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
    isMuted.store(other.isMuted.load());
    isSolo.store(other.isSolo.load());
    currentMagnitude.store(other.currentMagnitude.load());
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

  bool isTrackPlaying(InstrumentGroup group) const;

  float getTrackLevel(InstrumentGroup group) const;
  void setTrackLevel(InstrumentGroup group, float peak);

  void initializeTrack(InstrumentGroup group);

  // Called by AudioEngine inside processBlock
  void processBuffer(juce::AudioBuffer<float> &buffer, InstrumentGroup group);

private:
  std::map<InstrumentGroup, TrackState> tracks;
  bool anySoloActive = false;
  void updateSoloState();
  juce::CriticalSection lock;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerController)
};
