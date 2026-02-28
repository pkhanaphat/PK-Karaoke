#pragma once

#include <JuceHeader.h>
#include <map>

enum class InstrumentGroup
{
    // Melodic (CH1-28)
    Piano = 0, ChromaticPercussion, ThaiRanad, ThaiPongLang, Organ, Accordion, ThaiKaen,
    AcousticGuitarNylon, AcousticGuitarSteel, ElectricGuitarJazz, ElectricGuitarClean,
    OverdrivenGuitar, DistortionGuitar, Bass, Strings, Ensemble, Brass, Saxophone, Reed,
    Pipe, ThaiKlui, ThaiWote, SynthLead, SynthPad, SynthEffects, Ethnic, ThaiPoengMang, Percussive,

    // Drums (CH29-44)
    Kick = 100, Snare, HandClap, HiTom, MidTom, LowTom, HiHat, CrashCymbal, RideCymbal,
    Tambourine, Cowbell, Bongo, Conga, Timbale, ThaiChing, PercussionDrum,

    // Vocal
    VocalBus = 149,

    // VSTi
    VSTi1 = 150, VSTi2, VSTi3, VSTi4, VSTi5, VSTi6, VSTi7, VSTi8,

    // FX & Master
    ReverbBus = 200, DelayBus, ChorusBus, MasterBus
};

struct TrackState
{
    float volume = 1.0f; // Linear gain
    float pan = 0.5f;    // 0 = Left, 0.5 = Center, 1 = Right
    bool isMuted = false;
    bool isSolo = false;
};

class MixerController
{
public:
    MixerController();
    ~MixerController() = default;

    void setTrackVolume (InstrumentGroup group, float linearGain);
    float getTrackVolume (InstrumentGroup group) const;

    void setTrackPan (InstrumentGroup group, float panValue);
    float getTrackPan (InstrumentGroup group) const;

    void setTrackMute (InstrumentGroup group, bool shouldMute);
    bool isTrackMuted (InstrumentGroup group) const;

    void setTrackSolo (InstrumentGroup group, bool shouldSolo);
    bool isTrackSolo (InstrumentGroup group) const;

    // Apply the current mixer state to a block of audio for a specific group.
    // In a full implementation, this might be a custom AudioProcessor node in the graph.
    // For now, this is a helper to process a buffer.
    void processBuffer (juce::AudioBuffer<float>& buffer, InstrumentGroup group);

private:
    std::map<InstrumentGroup, TrackState> tracks;
    
    // Derived state caching
    bool anySoloActive = false;
    void updateSoloState();

    juce::CriticalSection lock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerController)
};

