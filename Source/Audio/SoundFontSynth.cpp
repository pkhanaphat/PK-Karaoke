#include "Audio/SoundFontSynth.h"

#define TSF_IMPLEMENTATION
#include "Audio/tsf.h"

SoundFontSynth::SoundFontSynth(MixerController *mixerController)
    : AudioProcessor(BusesProperties().withOutput(
          "Output", juce::AudioChannelSet::stereo(), true)),
      mixer(mixerController) {
  for (int i = 0; i < 16; ++i) {
    channelPrograms[i] = 0;
    isDrumChannel[i] = (i == 9); // Channel 10 is drums (0-indexed 9)
  }
}

SoundFontSynth::~SoundFontSynth() {
  const juce::SpinLock::ScopedLockType sl(lock);
  freeSynths();
}

void SoundFontSynth::freeSynths() {
  if (mainSynth != nullptr) {
    tsf_close(mainSynth);
    mainSynth = nullptr;
  }
}

bool SoundFontSynth::loadSoundFont(const juce::File &sf2File) {
  const juce::SpinLock::ScopedLockType sl(lock);
  freeSynths();

  if (currentSampleRate <= 0.0)
    currentSampleRate = 44100.0;

  if (sf2File.existsAsFile()) {
    mainSynth = tsf_load_filename(sf2File.getFullPathName().toUTF8());
    if (mainSynth != nullptr) {
      tsf_set_output(mainSynth, TSF_STEREO_INTERLEAVED, (int)currentSampleRate,
                     0.0f);
      return true;
    }
  }
  return false;
}

void SoundFontSynth::setVolume(float newVolume) {
  const juce::SpinLock::ScopedLockType sl(lock);
  currentVolume = newVolume;
  if (mainSynth != nullptr) {
    tsf_set_volume(mainSynth, currentVolume);
  }
}

void SoundFontSynth::prepareToPlay(double sampleRate, int samplesPerBlock) {
  const juce::SpinLock::ScopedLockType sl(lock);
  currentSampleRate = sampleRate;
  maxSamplesPerBlock = samplesPerBlock;
  interleavedBuffer.malloc(maxSamplesPerBlock * 2);

  if (mainSynth != nullptr) {
    tsf_set_output(mainSynth, TSF_STEREO_INTERLEAVED, (int)currentSampleRate,
                   0.0f);
    DBG("SoundFontSynth: prepareToPlay called. Sample Rate: "
        << currentSampleRate << ", Block Size: " << maxSamplesPerBlock);
  } else {
    DBG("SoundFontSynth: prepareToPlay called, but no SoundFont loaded yet.");
  }
}

void SoundFontSynth::releaseResources() {
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
  DBG("SoundFontSynth: releaseResources called.");
}

void SoundFontSynth::processBlock(juce::AudioBuffer<float> &buffer,
                                  juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  const juce::SpinLock::ScopedLockType sl(lock);

  if (mainSynth == nullptr) {
    buffer.clear();
    return;
  }

  int numSamples = buffer.getNumSamples();
  int currentSample = 0;

  float *interleaved = interleavedBuffer.get();

  for (const auto metadata : midiMessages) {
    const auto msg = metadata.getMessage();
    const int msgSample = metadata.samplePosition;

    int samplesToRender = msgSample - currentSample;
    if (samplesToRender > 0) {
      // 1. Render audio for the gap up to this MIDI message
      tsf_render_float(mainSynth, interleaved, samplesToRender, 0);

      // 2. Mix into buffer
      auto *outL = buffer.getWritePointer(0, currentSample);
      auto *outR = buffer.getWritePointer(1, currentSample);
      for (int i = 0; i < samplesToRender; ++i) {
        outL[i] += interleaved[i * 2] * currentVolume;
        outR[i] += interleaved[i * 2 + 1] * currentVolume;
      }

      currentSample += samplesToRender;
    }

    // Process MIDI Message into the synth
    int channel = msg.getChannel() - 1; // 0-15
    if (channel >= 0 && channel < 16) {
      if (msg.isNoteOn()) {
        tsf_channel_note_on(mainSynth, channel, msg.getNoteNumber(),
                            msg.getFloatVelocity());
      } else if (msg.isNoteOff()) {
        tsf_channel_note_off(mainSynth, channel, msg.getNoteNumber());
      } else if (msg.isProgramChange()) {
        int prog = msg.getProgramChangeNumber();
        channelPrograms[channel] = prog;
        tsf_channel_set_presetnumber(mainSynth, channel, prog, (channel == 9));
      } else if (msg.isPitchWheel()) {
        tsf_channel_set_pitchwheel(mainSynth, channel,
                                   msg.getPitchWheelValue());
      } else if (msg.isController()) {
        tsf_channel_midi_control(mainSynth, channel, msg.getControllerNumber(),
                                 msg.getControllerValue());
      }
    }
  }

  // Render remaining samples after the last MIDI event
  int remainingSamples = numSamples - currentSample;
  if (remainingSamples > 0) {
    tsf_render_float(mainSynth, interleaved, remainingSamples, 0);

    auto *outL = buffer.getWritePointer(0, currentSample);
    auto *outR = buffer.getWritePointer(1, currentSample);
    for (int i = 0; i < remainingSamples; ++i) {
      outL[i] += interleaved[i * 2] * currentVolume;
      outR[i] += interleaved[i * 2 + 1] * currentVolume;
    }
  }
}
