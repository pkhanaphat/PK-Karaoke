#include "KaraokeEngine.h"

KaraokeEngine::KaraokeEngine()
    : AudioProcessor (BusesProperties()
                     .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      graphManager(mixerController)
{
}

KaraokeEngine::~KaraokeEngine()
{
}

bool KaraokeEngine::loadSong (const juce::File& midiFile, const juce::File& lyrFile, const juce::File& curFile)
{
    DBG("KaraokeEngine: Loading song...");
    DBG("  MIDI: " << midiFile.getFullPathName());
    DBG("  LYR:  " << lyrFile.getFullPathName());
    DBG("  CUR:  " << curFile.getFullPathName());

    if (! midiFile.existsAsFile())
    {
        DBG("KaraokeEngine: MIDI file does not exist!");
        return false;
    }
    
    // CRITICAL: Load the MIDI file into the player!
    if (! midiPlayer.loadMidiFile (midiFile))
    {
        DBG("KaraokeEngine: Failed to load MIDI file into MidiPlayer!");
        return false;
    }

    // Get PPQ from MIDI file
    juce::FileInputStream stream (midiFile);
    int ppq = 120;
    if (stream.openedOk())
    {
        juce::MidiFile mf;
        if (mf.readFrom (stream))
            ppq = mf.getTimeFormat();
    }
    
    currentLyrics = ncnParser.parse (lyrFile, curFile, ppq);
    DBG("KaraokeEngine: Loaded song, PPQ=" << ppq << ", Lyrics Lines=" << currentLyrics.lines.size());
    
    return true;
}

void KaraokeEngine::play()
{
    DBG("KaraokeEngine: play() called. midiEvents=" << midiPlayer.getNumEvents() << ", isPlaying=" << (int)midiPlayer.isPlaying());
    if (midiPlayer.getNumEvents() == 0 && ! midiPlayer.isPlaying())
    {
        DBG("KaraokeEngine: Triggering checkQueue() from play()");
        checkQueue();
    }
    else
    {
        DBG("KaraokeEngine: Starting MidiPlayer playback.");
        midiPlayer.play();
    }
}

void KaraokeEngine::pause()
{
    midiPlayer.pause();
}

void KaraokeEngine::stop()
{
    midiPlayer.stop();
}

void KaraokeEngine::loadSoundFont (const juce::File& sf2File)
{
    graphManager.setSoundFont (sf2File);
}

void KaraokeEngine::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    graphManager.prepareToPlay (sampleRate, samplesPerBlock);
}

void KaraokeEngine::releaseResources()
{
    graphManager.releaseResources();
}

void KaraokeEngine::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // The main entry point for audio rendering
    // 1. Ask GraphManager to render the next block (it pulls events from MidiPlayer automatically)
    graphManager.getNextAudioBlock(buffer, buffer.getNumSamples(), getSampleRate(), midiPlayer);
    
    // 2. Apply Mixer Controller logic (Master Bus)
    mixerController.processBuffer(buffer, InstrumentGroup::MasterBus);

    // 3. Check for song completion to trigger queue
    bool isPlaying = midiPlayer.isPlaying();
    if (wasPlaying && !isPlaying)
    {
        DBG("KaraokeEngine: processBlock detected song finish. wasPlaying=true, isPlaying=false. Triggering checkQueue().");
        // Use a message manager call to avoid loading files on the audio thread
        juce::MessageManager::callAsync([this]() { checkQueue(); });
    }
    else if (!wasPlaying && isPlaying)
    {
        DBG("KaraokeEngine: processBlock detected song start. wasPlaying=false, isPlaying=true.");
    }
    wasPlaying = isPlaying;
}

void KaraokeEngine::checkQueue()
{
    SongRecord next;
    if (queueManager.popNext(next))
    {
        DBG("KaraokeEngine: Popped song from queue: " << next.title);
        juce::File midFile (next.path);
        
        juce::String lyrPathStr = next.path.replace ("\\Song\\", "\\Lyrics\\").replace ("\\song\\", "\\lyrics\\");
        lyrPathStr = lyrPathStr.upToLastOccurrenceOf (".", false, false) + ".lyr";
        juce::File lyrFile (lyrPathStr);
        if (! lyrFile.existsAsFile()) lyrFile = midFile.withFileExtension (".lyr");

        juce::String curPathStr = next.path.replace ("\\Song\\", "\\Cursor\\").replace ("\\song\\", "\\cursor\\");
        curPathStr = curPathStr.upToLastOccurrenceOf (".", false, false) + ".cur";
        juce::File curFile (curPathStr);
        if (! curFile.existsAsFile()) curFile = midFile.withFileExtension (".cur");

        if (loadSong (midFile, lyrFile, curFile))
        {
            DBG("KaraokeEngine: loadSong success. Triggering callback and play().");
            if (onSongLoaded)
                onSongLoaded();
            play();
        }
        else
        {
            DBG("KaraokeEngine: loadSong failed!");
        }
    }
    else
    {
        DBG("KaraokeEngine: Queue is empty.");
    }
}

void KaraokeEngine::getStateInformation (juce::MemoryBlock& destData) {}
void KaraokeEngine::setStateInformation (const void* data, int sizeInBytes) {}
