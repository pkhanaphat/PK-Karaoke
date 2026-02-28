#include "Core/Instruments/SF2Source.h"
#include "Core/MidiHelper.h"

#define TSF_IMPLEMENTATION
#include "Audio/tsf.h"

SF2Source::SF2Source(MixerController *mixerController)
    : AudioProcessor(BusesProperties().withOutput(
          "Output", juce::AudioChannelSet::stereo(), true)),
      mixer(mixerController) {
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
  if (mainSynth != nullptr) {
    tsf_close(mainSynth);
    mainSynth = nullptr;
  }
  for (int i = 0; i < 16; ++i)
    if (channelSynths[i] != nullptr) {
      tsf_close(channelSynths[i]);
      channelSynths[i] = nullptr;
    }
}

bool SF2Source::loadSoundFont(const juce::File &sf2File) {
  const juce::ScopedLock sl(lock);
  freeSynths();

  if (sf2File.existsAsFile()) {
    mainSynth = tsf_load_filename(sf2File.getFullPathName().toUTF8());
    if (mainSynth != nullptr) {
      tsf_set_output(mainSynth, TSF_STEREO_INTERLEAVED, (int)currentSampleRate,
                     0.0f);
      for (int i = 0; i < 16; ++i) {
        channelSynths[i] = tsf_copy(mainSynth);
        tsf_set_output(channelSynths[i], TSF_STEREO_INTERLEAVED,
                       (int)currentSampleRate, 0.0f);
      }
      DBG("SF2Source: Loaded " + sf2File.getFullPathName());
      return true;
    }
  }
  DBG("SF2Source: Failed to load " + sf2File.getFullPathName());
  return false;
}

void SF2Source::prepareToPlay(double sampleRate, int samplesPerBlock) {
  const juce::ScopedLock sl(lock);
  currentSampleRate = sampleRate;
  maxSamplesPerBlock = samplesPerBlock;
  interleavedBuffer.malloc(maxSamplesPerBlock * 2);

  if (mainSynth != nullptr) {
    tsf_set_output(mainSynth, TSF_STEREO_INTERLEAVED, (int)currentSampleRate,
                   0.0f);
    for (int i = 0; i < 16; ++i)
      if (channelSynths[i] != nullptr)
        tsf_set_output(channelSynths[i], TSF_STEREO_INTERLEAVED,
                       (int)currentSampleRate, 0.0f);
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
    for (int i = 0; i < 16; ++i)
      if (channelSynths[i] != nullptr)
        tsf_set_volume(channelSynths[i], currentVolume);
  }
}

void SF2Source::processBlock(juce::AudioBuffer<float> &buffer,
                             juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  const juce::ScopedLock sl(lock);

  if (mainSynth == nullptr) {
    buffer.clear();
    return;
  }

  const int numSamples = buffer.getNumSamples();
  int currentSample = 0;

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
  std::map<InstrumentGroup, float> groupPeaks;

  for (int ch = 0; ch < 16; ++ch) {
    if (channelSynths[ch] == nullptr)
      continue;

    tsf_render_float(channelSynths[ch], interleaved, numSamples, 0);

    InstrumentGroup group =
        isDrumChannel[ch] ? InstrumentGroup::PercussionDrum
                          : MidiHelper::getInstrumentType(channelPrograms[ch]);

    // Calculate peak magnitude for VU Meter
    float peak = 0.0f;
    for (int i = 0; i < numSamples * 2; ++i) {
      float absVal = std::abs(interleaved[i]);
      if (absVal > peak)
        peak = absVal;
    }

    groupPeaks[group] = std::max(groupPeaks[group], peak);

    float vol = currentVolume;
    float pan = 0.5f;

    if (mixer != nullptr) {
      if (!mixer->isTrackPlaying(group))
        vol = 0.0f;
      else
        vol *= mixer->getTrackVolume(group);
      pan = mixer->getTrackPan(group);
    }

    if (vol > 0.001f) {
      const float leftGain = vol * juce::jmin(1.0f, (1.0f - pan) * 2.0f);
      const float rightGain = vol * juce::jmin(1.0f, pan * 2.0f);

      auto *mixL = dest.getWritePointer(0, startSample);
      auto *mixR = dest.getWritePointer(1, startSample);

      for (int i = 0; i < numSamples; ++i) {
        mixL[i] += interleaved[i * 2] * leftGain;
        mixR[i] += interleaved[i * 2 + 1] * rightGain;
      }
    }
  }

  if (mixer != nullptr) {
    for (const auto &pair : groupPeaks) {
      mixer->setTrackLevel(pair.first, pair.second);
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

  if (msg.isNoteOn())
    tsf_channel_note_on(synth, channel, msg.getNoteNumber(),
                        msg.getFloatVelocity());
  else if (msg.isNoteOff())
    tsf_channel_note_off(synth, channel, msg.getNoteNumber());
  else if (msg.isProgramChange()) {
    const int prog = msg.getProgramChangeNumber();
    channelPrograms[channel] = prog;
    tsf_channel_set_presetnumber(synth, channel, prog, (channel == 9));
  } else if (msg.isPitchWheel())
    tsf_channel_set_pitchwheel(synth, channel, msg.getPitchWheelValue());
  else if (msg.isController())
    tsf_channel_midi_control(synth, channel, msg.getControllerNumber(),
                             msg.getControllerValue());
}
