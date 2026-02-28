#pragma once

#include <JuceHeader.h>
#include "AudioGraphManager.h"
#include "MidiPlayer.h"
#include "MixerController.h"
#include "NcnParser.h"
#include "QueueManager.h"

class KaraokeEngine : public juce::AudioProcessor
{
public:
    KaraokeEngine();
    ~KaraokeEngine() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    const juce::String getName() const override { return "Karaoke Engine"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const juce::String getProgramName (int index) override { return ""; }
    void changeProgramName (int index, const juce::String& newName) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Karaoke Actions
    bool loadSong (const juce::File& midiFile, const juce::File& lyrFile, const juce::File& curFile);
    void play();
    void pause();
    void stop();
    
    // SF2
    void loadSoundFont (const juce::File& sf2File);

    // Getters for UI
    MidiPlayer& getMidiPlayer() { return midiPlayer; }
    MixerController& getMixerController() { return mixerController; }
    QueueManager& getQueueManager() { return queueManager; }
    const SongLyrics& getCurrentLyrics() const { return currentLyrics; }
    
    std::function<void()> onSongLoaded;

private:
   MidiPlayer midiPlayer;
   MixerController mixerController;
   AudioGraphManager graphManager;
   NcnParser ncnParser;
   QueueManager queueManager;
   
   SongLyrics currentLyrics;

   bool wasPlaying = false;
   void checkQueue();

   JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KaraokeEngine)
};
