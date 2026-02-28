#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
    setLookAndFeel (&lookAndFeel);
    setSize (1000, 700);

    // UI Initialization
    addAndMakeVisible (lyricsView);
    addAndMakeVisible (songListView);
    addAndMakeVisible (youtubePlayer);
    addAndMakeVisible (queueComponent);
    
    addAndMakeVisible (loadDbButton);
    addAndMakeVisible (sf2Button);
    addAndMakeVisible (playButton);
    addAndMakeVisible (stopButton);
    addAndMakeVisible (mixerButton);

    lyricsView.attachToPlayer (&karaokeEngine.getMidiPlayer());
    
    karaokeEngine.onSongLoaded = [this]()
    {
        juce::MessageManager::callAsync([this]() {
            lyricsView.setLyrics (karaokeEngine.getCurrentLyrics());
        });
    };

    // Initialize Database
    juce::File dbFile = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                            .getChildFile ("PKKaraoke")
                            .getChildFile ("songs.db");
    
    if (dbManager.initialize (dbFile))
    {
        songListView.setDatabaseManager (&dbManager);
        DBG("Database OK, count: " + juce::String(dbManager.getTotalCount()));
    }

    // Set up Scanner Callbacks
    libraryScanner.onStatusChanged = [this] (juce::String status)
    {
        juce::MessageManager::callAsync ([this, status]()
        {
            // Update UI
            loadDbButton.setButtonText (status);
        });
    };

    libraryScanner.onScanFinished = [this] ()
    {
        juce::MessageManager::callAsync ([this]()
        {
            loadDbButton.setButtonText ("DB Created (" + juce::String(dbManager.getTotalCount()) + " songs)");
            songListView.refreshList();
        });
    };

    // Button callbacks
    loadDbButton.onClick = [this]()
    {
        chooser = std::make_unique<juce::FileChooser> ("Select NCN Karaoke Folder",
                                                       juce::File::getSpecialLocation (juce::File::userHomeDirectory));
        
        auto chooserFlags = juce::FileBrowserComponent::canSelectDirectories;

        chooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& fc)
        {
            auto dir = fc.getResult();
            if (dir.isDirectory())
            {
                juce::MessageManager::callAsync ([this, dir]()
                {
                    libraryScanner.startScan (dir);
                });
            }
        });
    };

    playButton.onClick = [this]() { karaokeEngine.play(); };
    stopButton.onClick = [this]() { karaokeEngine.stop(); };
    
    sf2Button.onClick = [this]()
    {
        chooser = std::make_unique<juce::FileChooser> ("Select SoundFont (.sf2)",
                                                       juce::File::getSpecialLocation (juce::File::userHomeDirectory),
                                                       "*.sf2");
        
        auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile())
            {
                juce::MessageManager::callAsync ([this, file]()
                {
                    karaokeEngine.loadSoundFont (file);
                    sf2Button.setButtonText (file.getFileName());
                });
            }
        });
    };

    mixerButton.onClick = [this]()
    {
        if (mixerWindow == nullptr)
            mixerWindow = std::make_unique<SynthMixerWindow> ("Synth Mixer", karaokeEngine.getMixerController());
        
        mixerWindow->setVisible (! mixerWindow->isVisible());
    };

    // List callback
    songListView.onSongSelected = [this](const SongRecord& entry)
    {
        bool wasEmpty = karaokeEngine.getQueueManager().isEmpty();
        karaokeEngine.getQueueManager().addSong(entry);
        
        // If nothing is playing and the queue was empty, start playing immediately
        if (wasEmpty && ! karaokeEngine.getMidiPlayer().isPlaying())
        {
            karaokeEngine.play();
        }
    };

    // Default API for Edge Backend
    // (Requires copying WebView2Loader.dll to build dir on Windows)

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    karaokeEngine.prepareToPlay (sampleRate, samplesPerBlockExpected);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // The AudioDeviceManager gives us a buffer, we pass it to our Engine processor
    juce::AudioBuffer<float> outputBuffer (bufferToFill.buffer->getArrayOfWritePointers(),
                                           bufferToFill.buffer->getNumChannels(),
                                           bufferToFill.buffer->getNumSamples());
                                           
    juce::MidiBuffer midiMessages;

    karaokeEngine.processBlock (outputBuffer, midiMessages);
}

void MainComponent::releaseResources()
{
    karaokeEngine.releaseResources();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    
    // Top Bar (Controls)
    auto topArea = area.removeFromTop (40).reduced(5);
    loadDbButton.setBounds (topArea.removeFromLeft (120));
    topArea.removeFromLeft(10);
    sf2Button.setBounds (topArea.removeFromLeft (120));
    topArea.removeFromLeft(10);
    playButton.setBounds (topArea.removeFromLeft (80));
    topArea.removeFromLeft(10);
    stopButton.setBounds (topArea.removeFromLeft (80));
    topArea.removeFromLeft(10);
    mixerButton.setBounds (topArea.removeFromLeft (80));

    // Split Left (List) and Right (Main View)
    auto leftArea = area.removeFromLeft (300);
    songListView.setBounds (leftArea.removeFromTop(leftArea.getHeight() / 2).reduced(5));
    queueComponent.setBounds (leftArea.reduced(5));

    // Split Right into Lyrics and Video
    auto videoArea = area.removeFromRight (area.getWidth() / 2);
    youtubePlayer.setBounds (videoArea.reduced(5));
    lyricsView.setBounds (area.reduced(5));
}
