#include "Core/Routing/MixerController.h"

MixerController::MixerController() {}

void MixerController::initializeTrack(InstrumentGroup group) {
  const juce::ScopedLock sl(lock);
  tracks[group];
}

bool MixerController::isTrackPlaying(InstrumentGroup group) const {
  const juce::ScopedLock sl(lock);
  auto it = tracks.find(group);
  if (it == tracks.end())
    return !anySoloActive;

  if (it->second.isMuted.load())
    return false;
  if (anySoloActive && !it->second.isSolo.load() &&
      group != InstrumentGroup::MasterBus)
    return false;

  return true;
}

void MixerController::setTrackVolume(InstrumentGroup group, float linearGain) {
  const juce::ScopedLock sl(lock);
  tracks[group].volume.store(juce::jlimit(0.0f, 2.0f, linearGain));
}

float MixerController::getTrackVolume(InstrumentGroup group) const {
  const juce::ScopedLock sl(lock);
  auto it = tracks.find(group);
  return it != tracks.end() ? it->second.volume.load() : 1.0f;
}

void MixerController::setTrackPan(InstrumentGroup group, float panValue) {
  const juce::ScopedLock sl(lock);
  tracks[group].pan.store(juce::jlimit(0.0f, 1.0f, panValue));
}

float MixerController::getTrackPan(InstrumentGroup group) const {
  const juce::ScopedLock sl(lock);
  auto it = tracks.find(group);
  return it != tracks.end() ? it->second.pan.load() : 0.5f;
}

void MixerController::setTrackGain(InstrumentGroup group, float linearGain) {
  const juce::ScopedLock sl(lock);
  tracks[group].gain.store(juce::jlimit(0.0f, 4.0f, linearGain));
}

float MixerController::getTrackGain(InstrumentGroup group) const {
  const juce::ScopedLock sl(lock);
  auto it = tracks.find(group);
  return it != tracks.end() ? it->second.gain.load() : 1.0f;
}

void MixerController::setTrackAuxSend(InstrumentGroup group, int auxIndex,
                                      float linearGain) {
  if (auxIndex < 0 || auxIndex >= 3)
    return;
  const juce::ScopedLock sl(lock);
  tracks[group].auxSends[auxIndex].store(juce::jlimit(0.0f, 2.0f, linearGain));
}

float MixerController::getTrackAuxSend(InstrumentGroup group,
                                       int auxIndex) const {
  if (auxIndex < 0 || auxIndex >= 3)
    return 0.0f;
  const juce::ScopedLock sl(lock);
  auto it = tracks.find(group);
  return it != tracks.end() ? it->second.auxSends[auxIndex].load() : 0.0f;
}

void MixerController::setTrackMute(InstrumentGroup group, bool shouldMute) {
  const juce::ScopedLock sl(lock);
  tracks[group].isMuted.store(shouldMute);
}

bool MixerController::isTrackMuted(InstrumentGroup group) const {
  const juce::ScopedLock sl(lock);
  auto it = tracks.find(group);
  return it != tracks.end() ? it->second.isMuted.load() : false;
}

void MixerController::setTrackSolo(InstrumentGroup group, bool shouldSolo) {
  const juce::ScopedLock sl(lock);
  tracks[group].isSolo.store(shouldSolo);
  updateSoloState();
}

bool MixerController::isTrackSolo(InstrumentGroup group) const {
  const juce::ScopedLock sl(lock);
  auto it = tracks.find(group);
  return it != tracks.end() ? it->second.isSolo.load() : false;
}

float MixerController::getTrackLevel(InstrumentGroup group) const {
  const juce::ScopedLock sl(lock);
  auto it = tracks.find(group);
  return it != tracks.end() ? it->second.currentMagnitude.load() : 0.0f;
}

void MixerController::setTrackLevel(InstrumentGroup group, float peak) {
  const juce::ScopedLock sl(lock);
  auto it = tracks.find(group);
  if (it != tracks.end()) {
    it->second.currentMagnitude.store(peak);
  }
}

void MixerController::updateSoloState() {
  anySoloActive = false;
  for (const auto &pair : tracks)
    if (pair.second.isSolo.load()) {
      anySoloActive = true;
      break;
    }
}

void MixerController::processBuffer(juce::AudioBuffer<float> &buffer,
                                    InstrumentGroup group) {
  const juce::ScopedLock sl(lock);
  auto it = tracks.find(group);
  if (it == tracks.end())
    return;

  auto &state = it->second;

  // 1. Apply Gain (Input Trim)
  float gValue = state.gain.load();
  if (gValue != 1.0f)
    buffer.applyGain(gValue);

  // 2. Track Level (Pre-fader magnitude)
  float mag = buffer.getMagnitude(0, buffer.getNumSamples());
  state.currentMagnitude.store(mag);

  // 3. Check Mute / Solo
  bool shouldPlay = true;
  if (state.isMuted.load())
    shouldPlay = false;
  else if (anySoloActive && !state.isSolo.load() &&
           group != InstrumentGroup::MasterBus)
    shouldPlay = false;

  if (!shouldPlay) {
    buffer.clear();
    return;
  }

  // 4. Apply Volume (Fader)
  float vValue = state.volume.load();
  if (vValue != 1.0f)
    buffer.applyGain(vValue);

  // 5. Apply Pan
  float pValue = state.pan.load();
  if (buffer.getNumChannels() == 2 && pValue != 0.5f) {
    buffer.applyGain(0, 0, buffer.getNumSamples(),
                     juce::jmin(1.0f, (1.0f - pValue) * 2.0f));
    buffer.applyGain(1, 0, buffer.getNumSamples(),
                     juce::jmin(1.0f, pValue * 2.0f));
  }
}
