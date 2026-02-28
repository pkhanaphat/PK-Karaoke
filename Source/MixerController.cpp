#include "MixerController.h"

MixerController::MixerController()
{
}

void MixerController::setTrackVolume (InstrumentGroup group, float linearGain)
{
    const juce::ScopedLock sl (lock);
    tracks[group].volume = juce::jlimit (0.0f, 2.0f, linearGain);
}

float MixerController::getTrackVolume (InstrumentGroup group) const
{
    const juce::ScopedLock sl (lock);
    auto it = tracks.find(group);
    return it != tracks.end() ? it->second.volume : 1.0f;
}

void MixerController::setTrackPan (InstrumentGroup group, float panValue)
{
    const juce::ScopedLock sl (lock);
    tracks[group].pan = juce::jlimit (0.0f, 1.0f, panValue);
}

float MixerController::getTrackPan (InstrumentGroup group) const
{
    const juce::ScopedLock sl (lock);
    auto it = tracks.find(group);
    return it != tracks.end() ? it->second.pan : 0.5f;
}

void MixerController::setTrackMute (InstrumentGroup group, bool shouldMute)
{
    const juce::ScopedLock sl (lock);
    tracks[group].isMuted = shouldMute;
}

bool MixerController::isTrackMuted (InstrumentGroup group) const
{
    const juce::ScopedLock sl (lock);
    auto it = tracks.find(group);
    return it != tracks.end() ? it->second.isMuted : false;
}

void MixerController::setTrackSolo (InstrumentGroup group, bool shouldSolo)
{
    const juce::ScopedLock sl (lock);
    tracks[group].isSolo = shouldSolo;
    updateSoloState();
}

bool MixerController::isTrackSolo (InstrumentGroup group) const
{
    const juce::ScopedLock sl (lock);
    auto it = tracks.find(group);
    return it != tracks.end() ? it->second.isSolo : false;
}

void MixerController::updateSoloState()
{
    anySoloActive = false;
    for (const auto& pair : tracks)
    {
        if (pair.second.isSolo)
        {
            anySoloActive = true;
            break;
        }
    }
}

void MixerController::processBuffer (juce::AudioBuffer<float>& buffer, InstrumentGroup group)
{
    const juce::ScopedLock sl (lock);
    auto it = tracks.find(group);
    
    if (it == tracks.end())
        return;

    const auto& state = it->second;

    // Check Mute/Solo logic
    bool shouldPlay = true;
    if (state.isMuted)
    {
        shouldPlay = false;
    }
    else if (anySoloActive && !state.isSolo && group != InstrumentGroup::MasterBus)
    {
        // If there's a solo active somewhere, and this track isn't soloed (and isn't the master bus), silence it.
        shouldPlay = false;
    }

    if (!shouldPlay)
    {
        buffer.clear();
        return;
    }

    // Apply Volume
    if (state.volume != 1.0f)
    {
        buffer.applyGain (state.volume);
    }

    // Apply Pan (assuming stereo buffer)
    if (buffer.getNumChannels() == 2 && state.pan != 0.5f)
    {
        float leftGain = juce::jmin (1.0f, (1.0f - state.pan) * 2.0f);
        float rightGain = juce::jmin (1.0f, state.pan * 2.0f);
        
        buffer.applyGain (0, 0, buffer.getNumSamples(), leftGain);
        buffer.applyGain (1, 0, buffer.getNumSamples(), rightGain);
    }
}
