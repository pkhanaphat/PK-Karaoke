#include "SoundFontSynth.h"

#define TSF_IMPLEMENTATION
#include "tsf.h"

SoundFontSynth::SoundFontSynth (MixerController* mixerController)
     : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       mixer (mixerController)
{
    for (int i = 0; i < 16; ++i)
    {
        channelSynths[i] = nullptr;
        channelPrograms[i] = 0;
        isDrumChannel[i] = (i == 9); // Channel 10 is drums (0-indexed 9)
    }
}

SoundFontSynth::~SoundFontSynth()
{
    const juce::ScopedLock sl (lock);
    freeSynths();
}

void SoundFontSynth::freeSynths()
{
    if (mainSynth != nullptr)
    {
        tsf_close (mainSynth);
        mainSynth = nullptr;
    }
    for (int i = 0; i < 16; ++i)
    {
        if (channelSynths[i] != nullptr)
        {
            tsf_close (channelSynths[i]);
            channelSynths[i] = nullptr;
        }
    }
}

bool SoundFontSynth::loadSoundFont (const juce::File& sf2File)
{
    const juce::ScopedLock sl (lock);
    freeSynths();

    if (sf2File.existsAsFile())
    {
        mainSynth = tsf_load_filename (sf2File.getFullPathName().toUTF8());
        if (mainSynth != nullptr)
        {
            tsf_set_output (mainSynth, TSF_STEREO_INTERLEAVED, (int)currentSampleRate, 0.0f);
            
            // Create 16 lightweight copies for multitimbral rendering
            for (int i = 0; i < 16; ++i)
            {
                channelSynths[i] = tsf_copy (mainSynth);
                tsf_set_output (channelSynths[i], TSF_STEREO_INTERLEAVED, (int)currentSampleRate, 0.0f);
            }
            return true;
        }
    }
    return false;
}

void SoundFontSynth::setVolume (float newVolume)
{
    const juce::ScopedLock sl (lock);
    currentVolume = newVolume;
    if (mainSynth != nullptr)
    {
        tsf_set_volume (mainSynth, currentVolume);
        for (int i = 0; i < 16; ++i)
        {
            if (channelSynths[i] != nullptr)
                tsf_set_volume (channelSynths[i], currentVolume);
        }
    }
}

void SoundFontSynth::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const juce::ScopedLock sl (lock);
    currentSampleRate = sampleRate;
    maxSamplesPerBlock = samplesPerBlock;
    interleavedBuffer.malloc (maxSamplesPerBlock * 2);
    
    if (mainSynth != nullptr)
    {
        tsf_set_output (mainSynth, TSF_STEREO_INTERLEAVED, (int)currentSampleRate, 0.0f);
        for (int i = 0; i < 16; ++i)
        {
            if (channelSynths[i] != nullptr)
                tsf_set_output (channelSynths[i], TSF_STEREO_INTERLEAVED, (int)currentSampleRate, 0.0f);
        }
        DBG("SoundFontSynth: prepareToPlay called. Sample Rate: " << currentSampleRate << ", Block Size: " << maxSamplesPerBlock);
    }
    else
    {
        DBG("SoundFontSynth: prepareToPlay called, but no SoundFont loaded yet.");
    }
}

void SoundFontSynth::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    DBG("SoundFontSynth: releaseResources called.");
}

void SoundFontSynth::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const juce::ScopedLock sl (lock);
    
    if (mainSynth == nullptr)
    {
        buffer.clear();
        return;
    }

    int numSamples = buffer.getNumSamples();
    int currentSample = 0;

    // Buffer for accumulating the final mix
    juce::AudioBuffer<float> mixBuffer (2, numSamples);
    mixBuffer.clear();

    // Intermediate buffer for rendering each channel chunk
    juce::AudioBuffer<float> tempBuffer (2, numSamples);

    static int logCounter = 0;
    if (midiMessages.getNumEvents() > 0)
    {
        DBG("SoundFontSynth: Processing " << midiMessages.getNumEvents() << " messages, Vol=" << currentVolume);
    }
    else if (++logCounter % 100 == 0)
    {
        // Periodic heartbeat to show the synth is alive
        // DBG("SoundFontSynth: Block processed (silent), Vol=" << currentVolume << ", SR=" << currentSampleRate);
    }

    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        const int msgSample = metadata.samplePosition;

        int samplesToRender = msgSample - currentSample;
        if (samplesToRender > 0)
        {
            // Render audio for the gap, Channel by Channel
            float* interleaved = interleavedBuffer.get();

            for (int ch = 0; ch < 16; ++ch)
            {
                if (channelSynths[ch] == nullptr) continue;

                // 1. Render this specific channel's audio
                tsf_render_float (channelSynths[ch], interleaved, samplesToRender, 0);

                InstrumentGroup group = InstrumentGroup::Piano;
                if (isDrumChannel[ch])
                {
                    group = InstrumentGroup::PercussionDrum;
                }
                else
                {
                    group = MidiHelper::getInstrumentType (channelPrograms[ch]);
                }

                // 3. Apply Mixer Volume/Pan/Mute
                float vol = currentVolume;
                float pan = 0.5f;
                bool isMuted = false;
                bool isSolo = false;

                if (mixer != nullptr)
                {
                    vol *= mixer->getTrackVolume (group);
                    pan = mixer->getTrackPan (group);
                    isMuted = mixer->isTrackMuted (group);
                    isSolo = mixer->isTrackSolo (group);
                    
                    // Simple solo logic check (assuming mixer handles the global solo state correctly,
                    // but we can query it directly here)
                    if (isMuted) vol = 0.0f;
                }

                // If volume is > 0, mix it into the main buffer
                if (vol > 0.001f)
                {
                    float leftGain = vol * juce::jmin (1.0f, (1.0f - pan) * 2.0f);
                    float rightGain = vol * juce::jmin (1.0f, pan * 2.0f);

                    auto* mixL = mixBuffer.getWritePointer (0, currentSample);
                    auto* mixR = mixBuffer.getWritePointer (1, currentSample);

                    for (int i = 0; i < samplesToRender; ++i)
                    {
                        mixL[i] += interleaved[i * 2] * leftGain;
                        mixR[i] += interleaved[i * 2 + 1] * rightGain;
                    }
                }
            }

            currentSample += samplesToRender;
        }

        // Process MIDI Message into the specific channel's synth
        int channel = msg.getChannel() - 1; // 0-15
        if (channel >= 0 && channel < 16)
        {
            if (msg.isNoteOn())
            {
                if (channelSynths[channel] != nullptr)
                    tsf_channel_note_on (channelSynths[channel], channel, msg.getNoteNumber(), msg.getFloatVelocity());
                else if (mainSynth != nullptr) // Fallback to mainSynth if channel synth is missing
                    tsf_channel_note_on (mainSynth, channel, msg.getNoteNumber(), msg.getFloatVelocity());
            }
            else if (msg.isNoteOff())
            {
                if (channelSynths[channel] != nullptr)
                    tsf_channel_note_off (channelSynths[channel], channel, msg.getNoteNumber());
                else if (mainSynth != nullptr) // Fallback to mainSynth if channel synth is missing
                    tsf_channel_note_off (mainSynth, channel, msg.getNoteNumber());
            }
            else if (msg.isProgramChange())
            {
                int prog = msg.getProgramChangeNumber();
                channelPrograms[channel] = prog;
                if (channelSynths[channel] != nullptr)
                    tsf_channel_set_presetnumber (channelSynths[channel], channel, prog, (channel == 9));
                else if (mainSynth != nullptr) // Fallback to mainSynth
                    tsf_channel_set_presetnumber (mainSynth, channel, prog, (channel == 9));
            }
            else if (msg.isPitchWheel())
            {
                if (channelSynths[channel] != nullptr)
                    tsf_channel_set_pitchwheel (channelSynths[channel], channel, msg.getPitchWheelValue());
                else if (mainSynth != nullptr) // Fallback to mainSynth
                    tsf_channel_set_pitchwheel (mainSynth, channel, msg.getPitchWheelValue());
            }
            else if (msg.isController())
            {
                if (channelSynths[channel] != nullptr)
                    tsf_channel_midi_control (channelSynths[channel], channel, msg.getControllerNumber(), msg.getControllerValue());
                else if (mainSynth != nullptr) // Fallback to mainSynth
                    tsf_channel_midi_control (mainSynth, channel, msg.getControllerNumber(), msg.getControllerValue());
            }
        }
        else
        {
            // SysEx or other global messages could be sent to all synths or just mainSynth
            // For now, we'll send to mainSynth as a fallback for unhandled messages
            if (mainSynth != nullptr)
            {
                // Example: if it's a System Exclusive message, send to mainSynth
                // tsf_midi_message(mainSynth, msg.getRawData(), msg.getRawDataSize());
            }
        }
    }

    // Render remaining samples after the last MIDI event
    int remainingSamples = numSamples - currentSample;
    if (remainingSamples > 0)
    {
        float* interleaved = interleavedBuffer.get();

        for (int ch = 0; ch < 16; ++ch)
        {
            if (channelSynths[ch] == nullptr) continue;

            tsf_render_float (channelSynths[ch], interleaved, remainingSamples, 0);

            InstrumentGroup group = isDrumChannel[ch] ? InstrumentGroup::PercussionDrum : MidiHelper::getInstrumentType (channelPrograms[ch]);

            float vol = currentVolume;
            float pan = 0.5f;
            if (mixer != nullptr)
            {
                if (mixer->isTrackMuted (group)) vol = 0.0f;
                else vol *= mixer->getTrackVolume (group);
                pan = mixer->getTrackPan (group);
            }

            if (vol > 0.001f)
            {
                float leftGain = vol * juce::jmin (1.0f, (1.0f - pan) * 2.0f);
                float rightGain = vol * juce::jmin (1.0f, pan * 2.0f);

                auto* mixL = mixBuffer.getWritePointer (0, currentSample);
                auto* mixR = mixBuffer.getWritePointer (1, currentSample);

                for (int i = 0; i < remainingSamples; ++i)
                {
                    mixL[i] += interleaved[i * 2] * leftGain;
                    mixR[i] += interleaved[i * 2 + 1] * rightGain;
                }
            }
        }
    }
    // 4. Fallback rendering for mainSynth if needed
    // (This ensures that if MIDI events were sent to mainSynth directly, they are heard)
    if (mainSynth != nullptr && remainingSamples > 0)
    {
        float* interleaved = interleavedBuffer.get();
        tsf_render_float (mainSynth, interleaved, remainingSamples, 0);
        
        auto* mixL = mixBuffer.getWritePointer (0, currentSample);
        auto* mixR = mixBuffer.getWritePointer (1, currentSample);
        
        for (int i = 0; i < remainingSamples; ++i)
        {
            mixL[i] += interleaved[i * 2] * currentVolume;
            mixR[i] += interleaved[i * 2 + 1] * currentVolume;
        }
    }

    // Finally, copy the mixed audio to the output buffer
    buffer.copyFrom (0, 0, mixBuffer, 0, 0, numSamples);
    buffer.copyFrom (1, 0, mixBuffer, 1, 0, numSamples);
}
