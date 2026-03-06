#include "Core/Instruments/SF2Source.h"
#include "Core/MidiHelper.h"
#include <JuceHeader.h>
#include <functional>
#include <map>
#include <optional>

#define TSF_IMPLEMENTATION
#include "Audio/tsf.h"

static void LOG_CRASH(const juce::String &msg) {
  DBG(msg);
}

SF2Source::SF2Source(MixerController *mixerController,
                     std::optional<InstrumentGroup> group)
    : AudioProcessor(
          BusesProperties()
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
              .withOutput("FX1", juce::AudioChannelSet::stereo(), true)
              .withOutput("FX2", juce::AudioChannelSet::stereo(), true)
              .withOutput("FX3", juce::AudioChannelSet::stereo(), true)),
      mixer(mixerController), targetGroup(group) {
  for (int i = 0; i < 16; ++i) {
    channelPrograms[i] = 0;
    isDrumChannel[i] = (i == 9);
  }
}

SF2Source::~SF2Source() {
  const juce::ScopedLock sl(lock);
  freeSynths();
}

void SF2Source::freeSynths() {
  LOG_CRASH("freeSynths: Start");
  if (mainSynth != nullptr) {
    tsf_close(mainSynth);
    mainSynth = nullptr;
  }
  for (auto &pair : activeSynths) {
    if (pair.second != nullptr) {
      tsf_close(pair.second);
    }
  }
  activeSynths.clear();
  // customSynths are managed by updateCustomSF2Routing currently, but they
  // should also be freed here
  for (auto &pair : customSynths) {
    if (pair.second != nullptr) {
      tsf_close(pair.second);
    }
  }
  customSynths.clear();
  LOG_CRASH("freeSynths: End");
}

bool SF2Source::loadSoundFont(const juce::File &sf2File, tsf *sharedSynth) {
  LOG_CRASH("loadSoundFont: Start");
  const juce::ScopedLock sl(lock);
  freeSynths();

  if (sf2File.existsAsFile()) {
    if (sharedSynth != nullptr) {
      mainSynth = tsf_copy(sharedSynth);
    } else {
      mainSynth = tsf_load_filename(sf2File.getFullPathName().toUTF8());
    }

    LOG_CRASH("TSF loaded: " +
              juce::String(mainSynth != nullptr ? "true" : "false"));

    if (mainSynth != nullptr) {
      loadedPath = sf2File.getFullPathName();
      tsf_set_output(mainSynth, TSF_STEREO_INTERLEAVED, (int)currentSampleRate,
                     0.0f);
      tsf_set_volume(mainSynth, currentVolume);

      // Pre-allocate synths for all possible sound-producing groups to avoid tsf_copy on the audio thread
      for (int i = 0; i <= 43; ++i) getOrCreateSynthForGroup(static_cast<InstrumentGroup>(i), mainSynth);
      for (int i = 100; i <= 116; ++i) getOrCreateSynthForGroup(static_cast<InstrumentGroup>(i), mainSynth);

      DBG("SF2Source: Loaded " + sf2File.getFullPathName());
      LOG_CRASH("loadSoundFont: End Success");
      return true;
    }
  }
  DBG("SF2Source: Failed to load " + sf2File.getFullPathName());
  LOG_CRASH("loadSoundFont: End Failed");
  return false;
}

void SF2Source::updateCustomSF2Routing(
    const std::map<InstrumentGroup, juce::String> &customPaths,
    std::function<tsf *(const juce::String &)> getSharedFontCallback) {
  LOG_CRASH("updateCustomSF2Routing: called with " +
            juce::String(customPaths.size()) + " paths");
  const juce::ScopedLock sl(lock);

  // Clear existing customs
  for (auto &pair : customSynths) {
    if (pair.second != nullptr) {
      tsf_close(pair.second);
    }
  }
  customSynths.clear();

  // Load new customs
  for (const auto &pair : customPaths) {
    if (pair.second.isNotEmpty() && juce::File(pair.second).existsAsFile()) {
      LOG_CRASH("updateCustomSF2Routing: getting shared font for " +
                pair.second);
      tsf *shared = getSharedFontCallback(pair.second);
      if (shared != nullptr) {
        LOG_CRASH("updateCustomSF2Routing: shared font retrieved, copying...");
        tsf *custom = tsf_copy(shared);
        if (custom != nullptr) {
          tsf_set_output(custom, TSF_STEREO_INTERLEAVED, (int)currentSampleRate,
                         0.0f);
          tsf_set_volume(custom, currentVolume);

          // Re-apply channel presets to the new synth so it has the correct
          // instrument state. Without this, fresh tsf_copy knows no program
          // change and tsf_channel_note_on plays nothing.
          for (int ch = 0; ch < 16; ++ch) {
            tsf_channel_set_presetnumber(custom, ch, channelPrograms[ch],
                                         (ch == 9));
          }

          customSynths[pair.first] = custom;
          LOG_CRASH("SF2Source: Loaded custom route for group " +
                    juce::String((int)pair.first) + " -> " + pair.second +
                    " (Shared: " +
                    juce::String::toHexString((juce::pointer_sized_int)shared) +
                    ", Custom: " +
                    juce::String::toHexString((juce::pointer_sized_int)custom) +
                    ")");
        } else {
          LOG_CRASH("updateCustomSF2Routing: failed to copy tsf");
        }
      } else {
        LOG_CRASH(
            "updateCustomSF2Routing: getSharedFontCallback returned nullptr");
      }
    } else {
      LOG_CRASH("updateCustomSF2Routing: path is empty or does not exist: " +
                pair.second);
    }
  }
}

void SF2Source::prepareToPlay(double sampleRate, int samplesPerBlock) {
  LOG_CRASH("prepareToPlay: Start");
  const juce::ScopedLock sl(lock);
  currentSampleRate = sampleRate;
  maxSamplesPerBlock = samplesPerBlock;
  interleavedBuffer.malloc(maxSamplesPerBlock * 2);
  internalMixBuffer.setSize(2, maxSamplesPerBlock);

  if (mainSynth != nullptr) {
    tsf_set_output(mainSynth, TSF_STEREO_INTERLEAVED, (int)currentSampleRate,
                   0.0f);
    for (auto &pair : activeSynths) {
      if (pair.second != nullptr) {
        tsf_set_output(pair.second, TSF_STEREO_INTERLEAVED,
                       (int)currentSampleRate, 0.0f);
      }
    }
  }

  for (auto &pair : customSynths) {
    if (pair.second != nullptr) {
      tsf_set_output(pair.second, TSF_STEREO_INTERLEAVED,
                     (int)currentSampleRate, 0.0f);
    }
  }
}

void SF2Source::releaseResources() {
  DBG("SF2Source: releaseResources called.");
}

void SF2Source::setSFVolume(float linearGain) {
  const juce::ScopedLock sl(lock);
  currentVolume = linearGain;
  if (mainSynth != nullptr) {
    tsf_set_volume(mainSynth, currentVolume);
    for (auto &pair : activeSynths) {
      if (pair.second != nullptr) {
        tsf_set_volume(pair.second, currentVolume);
      }
    }
  }
}

void SF2Source::processBlock(juce::AudioBuffer<float> &buffer,
                             juce::MidiBuffer &midiMessages) {
  LOG_CRASH("processBlock: Start");
  juce::ScopedNoDenormals noDenormals;
  const juce::ScopedLock sl(lock);

  // Only process if mainSynth exists
  if (mainSynth == nullptr) {
    buffer.clear();
    LOG_CRASH("processBlock: End (No Synth)");
    return;
  }

  const int numSamples = buffer.getNumSamples();
  int currentSample = 0;

  if (interleavedBuffer.getData() == nullptr ||
      numSamples > maxSamplesPerBlock) {
    if (numSamples > maxSamplesPerBlock) {
      maxSamplesPerBlock = numSamples;
    }
    interleavedBuffer.malloc(maxSamplesPerBlock * 2);
  }

  if (internalMixBuffer.getNumSamples() < numSamples) { internalMixBuffer.setSize(2, numSamples, false, true, true); }
  juce::AudioBuffer<float> mixBuffer(internalMixBuffer.getArrayOfWritePointers(), 2, numSamples);
  mixBuffer.clear();

  // Interleaved MIDI + audio render
  for (const auto metadata : midiMessages) {
    const int samplesToRender = metadata.samplePosition - currentSample;
    if (samplesToRender > 0) {
      renderChannels(mixBuffer, currentSample, samplesToRender);
      currentSample += samplesToRender;
    }
    processMidiMessage(metadata.getMessage());
  }

  const int remaining = numSamples - currentSample;
  if (remaining > 0)
    renderChannels(mixBuffer, currentSample, remaining);

  buffer.copyFrom(0, 0, mixBuffer, 0, 0, numSamples);
  buffer.copyFrom(1, 0, mixBuffer, 1, 0, numSamples);
}

void SF2Source::renderChannels(juce::AudioBuffer<float> &dest, int startSample,
                               int numSamples) {
  float *interleaved = interleavedBuffer.get();
  std::map<InstrumentGroup, std::pair<float, float>> groupPeaks;

  // Render from all actively playing dynamic synths + custom synths
  std::vector<InstrumentGroup> groupsToRender;
  for (const auto &pair : activeSynths) {
    if (pair.second != nullptr &&
        std::find(groupsToRender.begin(), groupsToRender.end(), pair.first) ==
            groupsToRender.end()) {
      groupsToRender.push_back(pair.first);
    }
  }
  for (const auto &pair : customSynths) {
    if (pair.second != nullptr &&
        std::find(groupsToRender.begin(), groupsToRender.end(), pair.first) ==
            groupsToRender.end()) {
      groupsToRender.push_back(pair.first);
    }
  }

  for (auto group : groupsToRender) {
    if (targetGroup.has_value() && targetGroup.value() != group)
      continue; // Skip rendering if it's not our target group

    if (std::find(excludedGroups.begin(), excludedGroups.end(), group) !=
        excludedGroups.end())
      continue; // Skip rendering if it's explicitly excluded

    tsf *synthToUse = nullptr;
    auto customIt = customSynths.find(group);
    if (customIt != customSynths.end() && customIt->second != nullptr) {
      synthToUse = customIt->second;
    } else {
      auto activeIt = activeSynths.find(group);
      if (activeIt != activeSynths.end() && activeIt->second != nullptr) {
        synthToUse = activeIt->second;
      }
    }

    if (synthToUse == nullptr)
      continue;

    // Clear interleaved buffer to prevent old audio from being mixed again
    // (fixes phantom Chorus/FX)
    std::fill(interleaved, interleaved + (numSamples * 2), 0.0f);

    tsf_render_float(synthToUse, interleaved, numSamples, 0);

    float vol = currentVolume;
    float pan = 0.5f;

    if (mixer != nullptr) {
      if (!targetGroup.has_value()) {
        // Global Synth Mode: Use mixer values per group
        if (!mixer->isTrackPlaying(group))
          vol = 0.0f;
        else
          vol *= mixer->getTrackVolume(group);
        pan = mixer->getTrackPan(group);
      } else {
        // Split Synth Mode: Output raw audio at unity (TrackProcessor handles
        // Vol/Pan later)
        vol = currentVolume;
        pan = 0.5f;
      }
    }

    // Calculate peak magnitude for VU Meter AFTER volume scaling and panning
    float peakL = 0.0f;
    float peakR = 0.0f;

    if (vol > 0.001f) {
      const float leftGain = vol * juce::jmin(1.0f, (1.0f - pan) * 2.0f);
      const float rightGain = vol * juce::jmin(1.0f, pan * 2.0f);

      auto *mixL = dest.getWritePointer(0, startSample);
      auto *mixR = dest.getWritePointer(1, startSample);

      // Get Aux Sends (Only for Global Synth mode)
      float sends[3] = {0.0f, 0.0f, 0.0f};
      bool shouldMixSendsInternally = !targetGroup.has_value();

      if (shouldMixSendsInternally && mixer != nullptr) {
        for (int i = 0; i < 3; ++i) {
          sends[i] = mixer->getTrackAuxSendBypass(group, i)
                         ? 0.0f
                         : mixer->getTrackAuxSend(group, i);
        }
      }

      for (int i = 0; i < numSamples; ++i) {
        float sampleL = interleaved[i * 2] * leftGain;
        float sampleR = interleaved[i * 2 + 1] * rightGain;

        mixL[i] += sampleL;
        mixR[i] += sampleR;

        // Mix to Aux Sends (FX1=ch2/3, FX2=ch4/5, FX3=ch6/7)
        if (shouldMixSendsInternally) {
          for (int auxIdx = 0; auxIdx < 3; ++auxIdx) {
            float sendVol = sends[auxIdx];
            if (sendVol > 0.001f) {
              int outChL = 2 + (auxIdx * 2);
              int outChR = outChL + 1;
              if (dest.getNumChannels() >= outChR + 1) {
                dest.getWritePointer(outChL, startSample)[i] +=
                    sampleL * sendVol;
                dest.getWritePointer(outChR, startSample)[i] +=
                    sampleR * sendVol;
              }
            }
          }
        }

        if (std::abs(sampleL) > peakL)
          peakL = std::abs(sampleL);
        if (std::abs(sampleR) > peakR)
          peakR = std::abs(sampleR);
      }
    } else {
      // If volume is 0, ensure aux channels are cleared if this is global mode
      if (!targetGroup.has_value()) {
        for (int ch = 2; ch < dest.getNumChannels(); ++ch) {
          dest.clear(ch, startSample, numSamples);
        }
      }
    }

    groupPeaks[group].first = std::max(groupPeaks[group].first, peakL);
    groupPeaks[group].second = std::max(groupPeaks[group].second, peakR);
  }

  if (mixer != nullptr) {
    for (const auto &pair : groupPeaks) {
      mixer->setTrackLevel(pair.first, pair.second.first, pair.second.second);
    }
  }
}

tsf *SF2Source::getOrCreateSynthForGroup(InstrumentGroup group,
                                         tsf *sharedFontToCopyFrom) {
  if (sharedFontToCopyFrom == nullptr)
    return nullptr;

  // Check custom first
  auto customIt = customSynths.find(group);
  if (customIt != customSynths.end() && customIt->second != nullptr) {
    return customIt->second;
  }

  // Check active
  auto activeIt = activeSynths.find(group);
  if (activeIt != activeSynths.end() && activeIt->second != nullptr) {
    return activeIt->second;
  }

  // To prevent audio thread allocation, if we are called during playback
  // we just return the mainSynth instead of cloning anything.
  // Initialization should happen during loadSoundFont().
  if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
      tsf *newSynth = tsf_copy(sharedFontToCopyFrom);
      if (newSynth != nullptr) {
        tsf_set_output(newSynth, TSF_STEREO_INTERLEAVED, (int)currentSampleRate, 0.0f);
        tsf_set_volume(newSynth, currentVolume);
        for (int ch = 0; ch < 16; ++ch) {
          tsf_channel_set_presetnumber(newSynth, ch, channelPrograms[ch], (ch == 9) ? 1 : 0);
        }
        activeSynths[group] = newSynth;
        return newSynth;
      }
  }

  return mainSynth;
}

void SF2Source::processMidiMessage(const juce::MidiMessage &msg) {
  const int channel = msg.getChannel() - 1;
  if (channel < 0 || channel >= 16)
    return;

  if (mainSynth == nullptr)
    return;

  InstrumentGroup group =
      MidiHelper::getInstrumentType(channelPrograms[channel]);
  if (isDrumChannel[channel] && msg.isNoteOnOrOff()) {
    group = MidiHelper::getInstrumentDrumType(msg.getNoteNumber());
  } else if (isDrumChannel[channel]) {
    // General drum channel message (CC, pitch) - apply to PercussionDrum by
    // default, though typically we broadcast these to all active drum synths
    // below.
    group = InstrumentGroup::PercussionDrum;
  }

  // If this synth instance is restricted to a specific group, ignore note
  // messages for other groups
  bool shouldPlayNotes =
      (!targetGroup.has_value() || targetGroup.value() == group) &&
      (std::find(excludedGroups.begin(), excludedGroups.end(), group) ==
       excludedGroups.end());

  int transpose = 0;
  if (mixer != nullptr && !isDrumChannel[channel]) {
    transpose = mixer->getTrackTranspose(group);
  }

  if (msg.isNoteOn()) {
    float velocity = msg.getFloatVelocity();
    tsf *synth = getOrCreateSynthForGroup(group, mainSynth);

    // Only play if not fully muted and matches target group
    if (synth != nullptr && velocity > 0.001f && shouldPlayNotes) {
      tsf_channel_note_on(synth, channel,
                          juce::jlimit(0, 127, msg.getNoteNumber() + transpose),
                          juce::jlimit(0.0f, 1.0f, velocity));
    }
  } else if (msg.isNoteOff()) {
    // Broadcast NoteOff to ALL synths to prevent hanging notes if a Program
    // Change occurred while the note was held (which changes the 'group'
    // routing).
    for (auto &pair : activeSynths) {
      if (pair.second != nullptr) {
        if (!targetGroup.has_value() || targetGroup.value() == pair.first) {
          int t = (mixer != nullptr && !isDrumChannel[channel])
                      ? mixer->getTrackTranspose(pair.first)
                      : 0;
          tsf_channel_note_off(pair.second, channel,
                               juce::jlimit(0, 127, msg.getNoteNumber() + t));
        }
      }
    }
    for (auto &pair : customSynths) {
      if (pair.second != nullptr) {
        if (!targetGroup.has_value() || targetGroup.value() == pair.first) {
          int t = (mixer != nullptr && !isDrumChannel[channel])
                      ? mixer->getTrackTranspose(pair.first)
                      : 0;
          tsf_channel_note_off(pair.second, channel,
                               juce::jlimit(0, 127, msg.getNoteNumber() + t));
        }
      }
    }
  } else if (msg.isProgramChange()) {
    const int prog = msg.getProgramChangeNumber();
    channelPrograms[channel] = prog;

    // Broadcast program change to all synths so they stay in sync
    for (auto &pair : activeSynths) {
      if (pair.second != nullptr)
        tsf_channel_set_presetnumber(pair.second, channel, prog,
                                     (channel == 9));
    }
    for (auto &pair : customSynths) {
      if (pair.second != nullptr)
        tsf_channel_set_presetnumber(pair.second, channel, prog,
                                     (channel == 9));
    }
  } else if (msg.isPitchWheel()) {
    // Broadcast to all synths
    for (auto &pair : activeSynths) {
      if (pair.second != nullptr)
        tsf_channel_set_pitchwheel(pair.second, channel,
                                   msg.getPitchWheelValue());
    }
    for (auto &pair : customSynths) {
      if (pair.second != nullptr)
        tsf_channel_set_pitchwheel(pair.second, channel,
                                   msg.getPitchWheelValue());
    }
  } else if (msg.isController()) {
    // Broadcast to all synths
    for (auto &pair : activeSynths) {
      if (pair.second != nullptr)
        tsf_channel_midi_control(pair.second, channel,
                                 msg.getControllerNumber(),
                                 msg.getControllerValue());
    }
    for (auto &pair : customSynths) {
      if (pair.second != nullptr)
        tsf_channel_midi_control(pair.second, channel,
                                 msg.getControllerNumber(),
                                 msg.getControllerValue());
    }
  }
}
