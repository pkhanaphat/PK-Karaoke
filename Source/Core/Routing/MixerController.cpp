#include "Core/Routing/MixerController.h"

MixerController::MixerController() {
  // Initialize mandatory buses
  initializeTrack(InstrumentGroup::MasterBus);
  initializeTrack(InstrumentGroup::ReverbBus);
  initializeTrack(InstrumentGroup::DelayBus);
  initializeTrack(InstrumentGroup::ChorusBus);
}

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

bool MixerController::isEffectivelyMuted(InstrumentGroup group) const {
  const juce::ScopedLock sl(lock);
  auto it = tracks.find(group);
  if (it == tracks.end())
    return true;

  if (it->second.isMuted.load())
    return true;

  if (anySoloActive && !it->second.isSolo.load())
    return true;

  return false;
}

float MixerController::getTrackLevelLeft(InstrumentGroup group) const {
  auto it = tracks.find(group);
  if (it != tracks.end())
    return it->second.currentPeakLeft.load();
  return 0.0f;
}

float MixerController::getTrackLevelRight(InstrumentGroup group) const {
  auto it = tracks.find(group);
  if (it != tracks.end())
    return it->second.currentPeakRight.load();
  return 0.0f;
}

void MixerController::setTrackLevel(InstrumentGroup group, float peakL,
                                    float peakR) {
  auto it = tracks.find(group);
  if (it != tracks.end()) {
    it->second.currentPeakLeft.store(peakL);
    it->second.currentPeakRight.store(peakR);
  }
}

void MixerController::setTrackTranspose(InstrumentGroup group,
                                        int transposeValue) {
  const juce::ScopedLock sl(lock);
  tracks[group].transpose.store(juce::jlimit(-24, 24, transposeValue));
}

int MixerController::getTrackTranspose(InstrumentGroup group) const {
  const juce::ScopedLock sl(lock);
  auto it = tracks.find(group);
  return it != tracks.end() ? it->second.transpose.load() : 0;
}

void MixerController::updateSoloState() {
  anySoloActive = false;
  for (const auto &pair : tracks)
    if (pair.second.isSolo.load()) {
      anySoloActive = true;
      break;
    }
}

void MixerController::setVstPluginPath(InstrumentGroup group, int slotIndex,
                                       const juce::String &path) {
  if (slotIndex < 0 || slotIndex >= 4)
    return;
  const juce::ScopedLock sl(lock);
  tracks[group].inserts[slotIndex].path = path;
}

juce::String MixerController::getVstPluginPath(InstrumentGroup group,
                                               int slotIndex) const {
  if (slotIndex < 0 || slotIndex >= 4)
    return {};
  const juce::ScopedLock sl(lock);
  auto it = tracks.find(group);
  return it != tracks.end() ? it->second.inserts[slotIndex].path
                            : juce::String{};
}

void MixerController::setVstPluginBypass(InstrumentGroup group, int slotIndex,
                                         bool bypass) {
  if (slotIndex < 0 || slotIndex >= 4)
    return;
  const juce::ScopedLock sl(lock);
  tracks[group].inserts[slotIndex].isBypass = bypass;
}

bool MixerController::getVstPluginBypass(InstrumentGroup group,
                                         int slotIndex) const {
  if (slotIndex < 0 || slotIndex >= 4)
    return false;
  const juce::ScopedLock sl(lock);
  auto it = tracks.find(group);
  return it != tracks.end() ? it->second.inserts[slotIndex].isBypass : false;
}

void MixerController::setTrackOutputDestination(InstrumentGroup group,
                                                OutputDestination dest) {
  const juce::ScopedLock sl(lock);
  if (tracks.find(group) == tracks.end()) {
    tracks[group]; // initialize if not exists
  }
  tracks[group].outputDest.store(dest);
}

OutputDestination
MixerController::getTrackOutputDestination(InstrumentGroup group) const {
  const juce::ScopedLock sl(lock);
  auto it = tracks.find(group);
  return it != tracks.end() ? it->second.outputDest.load()
                            : OutputDestination::SF2;
}

void MixerController::setTrackSF2Path(InstrumentGroup group,
                                      const juce::String &path) {
  const juce::ScopedLock sl(lock);
  tracks[group].sf2Path = path;
}

juce::String MixerController::getTrackSF2Path(InstrumentGroup group) const {
  const juce::ScopedLock sl(lock);
  auto it = tracks.find(group);
  return it != tracks.end() ? it->second.sf2Path : juce::String{};
}

void MixerController::setSF2Directory(const juce::File &directory) {
  const juce::ScopedLock sl(lock);
  sf2Directory = directory;
  updateSF2List();
}

void MixerController::updateSF2List() {
  const juce::ScopedLock sl(lock);
  availableSf2Files.clear();
  if (sf2Directory.isDirectory()) {
    availableSf2Files =
        sf2Directory.findChildFiles(juce::File::findFiles, true, "*.sf2");
  }
}

juce::StringArray MixerController::getAvailableSF2Names() const {
  const juce::ScopedLock sl(lock);
  juce::StringArray names;
  names.add("SF2 (Default)");
  for (auto &file : availableSf2Files) {
    names.add(file.getFileNameWithoutExtension());
  }
  return names;
}

juce::File MixerController::getSF2FileByName(const juce::String &name) const {
  const juce::ScopedLock sl(lock);
  for (auto &file : availableSf2Files) {
    if (file.getFileNameWithoutExtension() == name)
      return file;
  }
  return juce::File();
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

  // 2. Track Level (Post-fader magnitude, calculated at the end of function)

  // 3. Check Mute / Solo
  bool shouldPlay = true;
  if (state.isMuted.load())
    shouldPlay = false;
  else if (anySoloActive && !state.isSolo.load() &&
           group != InstrumentGroup::MasterBus)
    shouldPlay = false;

  if (!shouldPlay) {
    buffer.clear();
    state.currentPeakLeft.store(0.0f);
    state.currentPeakRight.store(0.0f);
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

  // Peak calculation is now stored at the very end of processBuffer
  // (Post-fader)
  float magL = buffer.getMagnitude(0, 0, buffer.getNumSamples());
  float magR = buffer.getNumChannels() > 1
                   ? buffer.getMagnitude(1, 0, buffer.getNumSamples())
                   : magL;
  state.currentPeakLeft.store(magL);
  state.currentPeakRight.store(magR);
}
