#include "Core/Instruments/SF2Source.h"
#include "Core/MidiHelper.h"
#include <JuceHeader.h>
#include <functional>
#include <map>
#include <optional>

#define TSF_IMPLEMENTATION
#include "Audio/tsf.h"

static void LOG_CRASH(const juce::String &msg) {
  juce::File("C:\\temp\\crash.log").appendText(msg + "\n");
}

SF2Source::SF2Source(MixerController *mixerController,
                     std::optional<InstrumentGroup> group)
    : AudioProcessor(BusesProperties().withOutput(
          "Output", juce::AudioChannelSet::stereo(), true)),
      mixer(mixerController), targetGroup(group) {
  for (int i = 0; i < 16; ++i) {
    channelSynths[i] = nullptr;
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
  for (int i = 0; i < 16; ++i) {
    if (channelSynths[i] != nullptr) {
      tsf_close(channelSynths[i]);
      channelSynths[i] = nullptr;
    }
  }
  for (auto &pair : drumSynths) {
    if (pair.second != nullptr) {
      tsf_close(pair.second);
    }
  }
  drumSynths.clear();
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
      for (int i = 0; i < 16; ++i) {
        channelSynths[i] = tsf_copy(mainSynth);
        tsf_set_output(channelSynths[i], TSF_STEREO_INTERLEAVED,
                       (int)currentSampleRate, 0.0f);
      }

      // Initialize separate synths for each drum group
      std::vector<InstrumentGroup> drumGroups = {
          InstrumentGroup::Kick,        InstrumentGroup::Snare,
          InstrumentGroup::HiHat,       InstrumentGroup::HiTom,
          InstrumentGroup::MidTom,      InstrumentGroup::LowTom,
          InstrumentGroup::CrashCymbal, InstrumentGroup::RideCymbal,
          InstrumentGroup::HandClap,    InstrumentGroup::Tambourine,
          InstrumentGroup::Cowbell,     InstrumentGroup::Bongo,
          InstrumentGroup::Conga,       InstrumentGroup::Timbale,
          InstrumentGroup::ThaiChing,   InstrumentGroup::PercussionDrum};
      for (auto group : drumGroups) {
        if (!targetGroup.has_value() || targetGroup.value() == group) {
          drumSynths[group] = tsf_copy(mainSynth);
          tsf_set_output(drumSynths[group], TSF_STEREO_INTERLEAVED,
                         (int)currentSampleRate, 0.0f);
        }
      }

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

  if (mainSynth != nullptr) {
    tsf_set_output(mainSynth, TSF_STEREO_INTERLEAVED, (int)currentSampleRate,
                   0.0f);
    for (int i = 0; i < 16; ++i) {
      if (channelSynths[i] != nullptr) {
        tsf_set_output(channelSynths[i], TSF_STEREO_INTERLEAVED,
                       (int)currentSampleRate, 0.0f);
      }
    }
    for (auto &pair : drumSynths) {
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
    for (int i = 0; i < 16; ++i) {
      if (channelSynths[i] != nullptr) {
        tsf_set_volume(channelSynths[i], currentVolume);
      }
    }
    for (auto &pair : drumSynths) {
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

  if (mainSynth == nullptr) {
    buffer.clear();
    LOG_CRASH("processBlock: End (No Synth)");
    return;
  }

  // Double check that our internal arrays are not partially initialized during
  // a race
  for (int i = 0; i < 16; ++i) {
    if (channelSynths[i] == nullptr) {
      buffer.clear();
      return;
    }
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

  juce::AudioBuffer<float> mixBuffer(2, numSamples);
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

  for (int ch = 0; ch < 16; ++ch) {
    if (isDrumChannel[ch] || channelSynths[ch] == nullptr)
      continue;

    InstrumentGroup group = MidiHelper::getInstrumentType(channelPrograms[ch]);
    if (targetGroup.has_value() && targetGroup.value() != group)
      continue; // Skip rendering if it's not our target group

    tsf *synthToUse = channelSynths[ch];
    auto customIt = customSynths.find(group);
    if (customIt != customSynths.end() && customIt->second != nullptr) {
      synthToUse = customIt->second;
    }

    if (synthToUse == nullptr) {
      LOG_CRASH("SF2Source: synthToUse is nullptr for channel " +
                juce::String(ch));
      continue;
    }

    tsf_render_float(synthToUse, interleaved, numSamples, 0);

    float vol = currentVolume;
    float pan = 0.5f;

    if (mixer != nullptr) {
      if (!mixer->isTrackPlaying(group))
        vol = 0.0f;
      else
        vol *= mixer->getTrackVolume(group);
      pan = mixer->getTrackPan(group);
    }

    // Calculate peak magnitude for VU Meter AFTER volume scaling and panning
    float peakL = 0.0f;
    float peakR = 0.0f;

    if (vol > 0.001f) {
      const float leftGain = vol * juce::jmin(1.0f, (1.0f - pan) * 2.0f);
      const float rightGain = vol * juce::jmin(1.0f, pan * 2.0f);

      auto *mixL = dest.getWritePointer(0, startSample);
      auto *mixR = dest.getWritePointer(1, startSample);

      bool hasAudio = false;
      for (int i = 0; i < numSamples; ++i) {
        float sampleL = interleaved[i * 2] * leftGain;
        float sampleR = interleaved[i * 2 + 1] * rightGain;

        mixL[i] += sampleL;
        mixR[i] += sampleR;

        if (std::abs(sampleL) > peakL)
          peakL = std::abs(sampleL);
        if (std::abs(sampleR) > peakR)
          peakR = std::abs(sampleR);

        if (std::abs(sampleL) > 0.0001f || std::abs(sampleR) > 0.0001f)
          hasAudio = true;
      }

      static int audioPulseCount = 0;
      if (hasAudio) {
        if (++audioPulseCount % 50 == 0)
          LOG_CRASH("SF2Source: Rendered AUDIO data for channel " +
                    juce::String(ch) + " vol: " + juce::String(vol) +
                    " leftGain: " + juce::String(leftGain) +
                    " rightGain: " + juce::String(rightGain));
      }
    }

    groupPeaks[group].first = std::max(groupPeaks[group].first, peakL);
    groupPeaks[group].second = std::max(groupPeaks[group].second, peakR);
  }

  // Render separate Drum buses
  for (auto &pair : drumSynths) {
    auto group = pair.first;
    auto synthToUse = pair.second;

    auto customIt = customSynths.find(group);
    if (customIt != customSynths.end() && customIt->second != nullptr) {
      synthToUse = customIt->second;
    }

    if (synthToUse == nullptr)
      continue;

    tsf_render_float(synthToUse, interleaved, numSamples, 0);

    float vol = currentVolume;
    float pan = 0.5f;

    if (mixer != nullptr) {
      if (!mixer->isTrackPlaying(group))
        vol = 0.0f;
      else
        vol *= mixer->getTrackVolume(group);
      pan = mixer->getTrackPan(group);
    }

    // Calculate peak magnitude for VU Meter AFTER volume scaling and panning
    float peakL = 0.0f;
    float peakR = 0.0f;

    if (vol > 0.001f) {
      const float leftGain = vol * juce::jmin(1.0f, (1.0f - pan) * 2.0f);
      const float rightGain = vol * juce::jmin(1.0f, pan * 2.0f);

      auto *mixL = dest.getWritePointer(0, startSample);
      auto *mixR = dest.getWritePointer(1, startSample);

      for (int i = 0; i < numSamples; ++i) {
        float sampleL = interleaved[i * 2] * leftGain;
        float sampleR = interleaved[i * 2 + 1] * rightGain;

        mixL[i] += sampleL;
        mixR[i] += sampleR;

        if (std::abs(sampleL) > peakL)
          peakL = std::abs(sampleL);
        if (std::abs(sampleR) > peakR)
          peakR = std::abs(sampleR);
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

void SF2Source::processMidiMessage(const juce::MidiMessage &msg) {
  const int channel = msg.getChannel() - 1;
  if (channel < 0 || channel >= 16)
    return;

  tsf *synth =
      channelSynths[channel] != nullptr ? channelSynths[channel] : mainSynth;
  if (synth == nullptr)
    return;

  InstrumentGroup group =
      MidiHelper::getInstrumentType(channelPrograms[channel]);
  if (isDrumChannel[channel]) {
    group = MidiHelper::getInstrumentDrumType(msg.getNoteNumber());
  }

  // If this synth instance is restricted to a specific group, ignore messages
  // for other groups However, we still need to process program changes and
  // control changes for all channels to maintain sync with the MIDI stream, so
  // we only restrict note on/off.
  bool shouldPlayNotes =
      !targetGroup.has_value() || targetGroup.value() == group;

  int transpose = 0;
  if (mixer != nullptr && !isDrumChannel[channel]) {
    transpose = mixer->getTrackTranspose(group);
  }

  if (msg.isNoteOn()) {
    float velocity = msg.getFloatVelocity();

    // Determine which synth instance to use
    bool usedCustom = false;
    auto customIt = customSynths.find(group);
    if (customIt != customSynths.end() && customIt->second != nullptr) {
      synth = customIt->second;
      usedCustom = true;
    } else if (isDrumChannel[channel]) {
      auto it = drumSynths.find(group);
      if (it != drumSynths.end() && it->second != nullptr) {
        synth = it->second;
      }
    }

    // Only play if not fully muted and matches target group
    if (velocity > 0.001f && shouldPlayNotes) {
      static int noteCount = 0;
      if (++noteCount % 10 == 0)
        LOG_CRASH("SF2Source: Note ON channel " + juce::String(channel) +
                  " note " + juce::String(msg.getNoteNumber()) + " vol " +
                  juce::String(currentVolume));
      tsf_channel_note_on(synth, channel,
                          juce::jlimit(0, 127, msg.getNoteNumber() + transpose),
                          juce::jlimit(0.0f, 1.0f, velocity));
    }
  } else if (msg.isNoteOff()) {
    bool usedCustom = false;
    auto customIt = customSynths.find(group);
    if (customIt != customSynths.end() && customIt->second != nullptr) {
      synth = customIt->second;
      usedCustom = true;
    } else if (isDrumChannel[channel]) {
      auto it = drumSynths.find(group);
      if (it != drumSynths.end() && it->second != nullptr) {
        synth = it->second;
      }
    }

    if (shouldPlayNotes) {
      tsf_channel_note_off(
          synth, channel,
          juce::jlimit(0, 127, msg.getNoteNumber() + transpose));
    }
  } else if (msg.isProgramChange()) {
    const int prog = msg.getProgramChangeNumber();
    channelPrograms[channel] = prog;
    tsf_channel_set_presetnumber(synth, channel, prog, (channel == 9));

    if (isDrumChannel[channel]) {
      for (auto &pair : drumSynths) {
        tsf_channel_set_presetnumber(pair.second, channel, prog, 1);
      }
    }
    for (auto &pair : customSynths) {
      tsf_channel_set_presetnumber(pair.second, channel, prog,
                                   isDrumChannel[channel] ? 1 : 0);
    }
  } else if (msg.isPitchWheel()) {
    tsf_channel_set_pitchwheel(synth, channel, msg.getPitchWheelValue());
    if (isDrumChannel[channel]) {
      for (auto &pair : drumSynths) {
        tsf_channel_set_pitchwheel(pair.second, channel,
                                   msg.getPitchWheelValue());
      }
    }
    for (auto &pair : customSynths) {
      tsf_channel_set_pitchwheel(pair.second, channel,
                                 msg.getPitchWheelValue());
    }
  } else if (msg.isController()) {
    tsf_channel_midi_control(synth, channel, msg.getControllerNumber(),
                             msg.getControllerValue());
    if (isDrumChannel[channel]) {
      for (auto &pair : drumSynths) {
        tsf_channel_midi_control(pair.second, channel,
                                 msg.getControllerNumber(),
                                 msg.getControllerValue());
      }
    }
    for (auto &pair : customSynths) {
      tsf_channel_midi_control(pair.second, channel, msg.getControllerNumber(),
                               msg.getControllerValue());
    }
  }
}
