#pragma once

#include <JuceHeader.h>
#include "KaraokeEngine.h"
#include "LyricsView.h"
#include "SongListView.h"
#include "YouTubePlayerComponent.h"
#include "YouTubeService.h"
#include "DatabaseManager.h"
#include "LibraryScanner.h"
#include "QueueComponent.h"
#include "SynthMixerWindow.h"
#include "LookAndFeel_PK.h"

class MainComponent  : public juce::AudioAppComponent
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    KaraokeEngine karaokeEngine;
    YouTubeService youtubeService;
    
    // UI Components
    LyricsView lyricsView;
    SongListView songListView;
    YouTubePlayerComponent youtubePlayer;
    QueueComponent queueComponent { karaokeEngine.getQueueManager() };
    
    juce::TextButton loadDbButton { "Load NCN Folder" };
    juce::TextButton sf2Button { "Select SoundFont" };
    juce::TextButton playButton { "Play" };
    juce::TextButton stopButton { "Stop" };
    juce::TextButton mixerButton { "Mixer" };

    std::unique_ptr<SynthMixerWindow> mixerWindow;
    std::unique_ptr<juce::FileChooser> chooser;
    
    LookAndFeel_PK lookAndFeel;

    DatabaseManager dbManager;
    LibraryScanner libraryScanner { dbManager };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
