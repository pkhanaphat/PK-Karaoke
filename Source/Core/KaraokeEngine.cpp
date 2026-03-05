#include "Core/KaraokeEngine.h"

KaraokeEngine::KaraokeEngine()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      graphManager(mixerController) {}

KaraokeEngine::~KaraokeEngine() { cancelPendingUpdate(); }

bool KaraokeEngine::loadSong(const juce::File &midiFile,
                             const juce::File &lyrFile,
                             const juce::File &curFile) {
  DBG("KaraokeEngine: Loading song...");
  DBG("  MIDI: " << midiFile.getFullPathName());
  DBG("  LYR:  " << lyrFile.getFullPathName());
  DBG("  CUR:  " << curFile.getFullPathName());

  if (!midiFile.existsAsFile()) {
    DBG("KaraokeEngine: MIDI file does not exist!");
    return false;
  }

  // CRITICAL: Load the MIDI file into the player!
  if (!midiPlayer.loadMidiFile(midiFile)) {
    DBG("KaraokeEngine: Failed to load MIDI file into MidiPlayer!");
    return false;
  }

  // Get PPQ from MIDI file
  juce::FileInputStream stream(midiFile);
  int ppq = 120;
  if (stream.openedOk()) {
    juce::MidiFile mf;
    if (mf.readFrom(stream)) {
      short tf = mf.getTimeFormat();
      if (tf > 0)
        ppq = tf;
      else
        ppq = 480; // Fallback for SMPTE
    }
  }

  currentLyrics = ncnParser.parse(lyrFile, curFile, ppq);
  DBG("KaraokeEngine: Loaded song, PPQ=" << ppq << ", Lyrics Lines="
                                         << currentLyrics.lines.size());

  return true;
}

void KaraokeEngine::play() {
  DBG("KaraokeEngine: play() called. midiEvents="
      << midiPlayer.getNumEvents()
      << ", isPlaying=" << (int)midiPlayer.isPlaying());
  if (midiPlayer.getNumEvents() == 0 && !midiPlayer.isPlaying()) {
    DBG("KaraokeEngine: Triggering checkQueue() from play()");
    checkQueue();
  } else {
    DBG("KaraokeEngine: Starting MidiPlayer playback.");
    midiPlayer.play();
  }
}

void KaraokeEngine::pause() { midiPlayer.pause(); }

void KaraokeEngine::stop() {
  midiPlayer.stop();
  graphManager.resetSynthesizers(); // Explicitly clear hanging
                                    // notes/controllers on stop
}

void KaraokeEngine::nextTrack() {
  stop();
  triggerAsyncUpdate(); // Safe trigger on audio thread or UI thread
}

void KaraokeEngine::loadSoundFont(const juce::File &sf2File) {
  graphManager.setSoundFont(sf2File);
}

void KaraokeEngine::prepareToPlay(double sampleRate, int samplesPerBlock) {
  graphManager.prepareToPlay(sampleRate, samplesPerBlock);
}

void KaraokeEngine::releaseResources() { graphManager.releaseResources(); }

void KaraokeEngine::handleAsyncUpdate() { checkQueue(); }

void KaraokeEngine::processBlock(juce::AudioBuffer<float> &buffer,
                                 juce::MidiBuffer &midiMessages) {
  // Clear buffer to prevent audio garbage
  buffer.clear();

  // The main entry point for audio rendering
  // 1. Ask GraphManager to render the next block (it pulls events from
  // MidiPlayer automatically)
  graphManager.getNextAudioBlock(buffer, buffer.getNumSamples(),
                                 getSampleRate(), midiPlayer);

  // 2. Apply Mixer Controller logic (Master Bus)
  mixerController.processBuffer(buffer, InstrumentGroup::MasterBus);

  // 3. Check for song completion natively vs user manual stop
  if (midiPlayer.hasFinishedPlayback()) {
    DBG("KaraokeEngine: processBlock detected song physically finished. "
        "Triggering checkQueue().");
    midiPlayer.clearFinishedPlayback();
    triggerAsyncUpdate();
  }

  bool isPlaying = midiPlayer.isPlaying();
  if (!wasPlaying && isPlaying) {
    DBG("KaraokeEngine: processBlock detected song start. wasPlaying=false, "
        "isPlaying=true.");
  }
  wasPlaying = isPlaying;
}

void KaraokeEngine::checkQueue() {
  SongRecord next;
  if (queueManager.popNext(next)) {
    DBG("KaraokeEngine: Popped song from queue: " << next.title);
    juce::File midFile(next.path);

    // NCN folders usually structured as:
    // Root/Songs/[A-Z]/12345.mid
    // Root/Lyrics/[A-Z]/12345.lyr
    // Root/Cursor/[A-Z]/12345.cur

    juce::File songSubDir = midFile.getParentDirectory();  // e.g. "A"
    juce::File songsDir = songSubDir.getParentDirectory(); // e.g. "Songs"
    juce::File rootDir = songsDir.getParentDirectory();    // e.g. "Root"

    // 1. Try Root/Lyrics/[A-Z]/filename.lyr
    juce::File lyrFile =
        rootDir.getChildFile("Lyrics")
            .getChildFile(songSubDir.getFileName())
            .getChildFile(midFile.getFileNameWithoutExtension() + ".lyr");

    if (!lyrFile.existsAsFile()) {
      // 2. Try Root/lyrics/[A-Z]/filename.lyr (lowercase case sensitivity)
      lyrFile =
          rootDir.getChildFile("lyrics")
              .getChildFile(songSubDir.getFileName())
              .getChildFile(midFile.getFileNameWithoutExtension() + ".lyr");
    }

    if (!lyrFile.existsAsFile()) {
      // 3. Try Root/Lyrics/filename.lyr (flat)
      lyrFile = rootDir.getChildFile("Lyrics").getChildFile(
          midFile.getFileNameWithoutExtension() + ".lyr");
    }

    if (!lyrFile.existsAsFile()) {
      // 4. Try Same folder as MIDI
      lyrFile = midFile.withFileExtension(".lyr");
    }

    // -- Cursor --
    juce::File curFile =
        rootDir.getChildFile("Cursor")
            .getChildFile(songSubDir.getFileName())
            .getChildFile(midFile.getFileNameWithoutExtension() + ".cur");

    if (!curFile.existsAsFile()) {
      curFile =
          rootDir.getChildFile("cursor")
              .getChildFile(songSubDir.getFileName())
              .getChildFile(midFile.getFileNameWithoutExtension() + ".cur");
    }

    if (!curFile.existsAsFile()) {
      curFile = rootDir.getChildFile("Cursor").getChildFile(
          midFile.getFileNameWithoutExtension() + ".cur");
    }

    if (!curFile.existsAsFile()) {
      curFile = midFile.withFileExtension(".cur");
    }

    if (loadSong(midFile, lyrFile, curFile)) {
      DBG("KaraokeEngine: loadSong success. Triggering callback and play().");

      // Reset synths right before playing the new song to clear old
      // bank/program states
      graphManager.resetSynthesizers();

      if (onSongLoaded)
        onSongLoaded();
      play();
    } else {
      DBG("KaraokeEngine: loadSong failed!");
    }
  } else {
    DBG("KaraokeEngine: Queue is empty.");
  }
}

void KaraokeEngine::getStateInformation(juce::MemoryBlock &destData) {}
void KaraokeEngine::setStateInformation(const void *data, int sizeInBytes) {}
