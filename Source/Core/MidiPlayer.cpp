#include "Core/MidiPlayer.h"

MidiPlayer::MidiPlayer() {}

bool MidiPlayer::loadMidiFile(const juce::File &file) {
  const juce::ScopedLock sl(lock);

  juce::FileInputStream stream(file);
  if (stream.openedOk()) {
    juce::MidiFile midiFile;
    if (midiFile.readFrom(stream)) {
      midiFile.convertTimestampTicksToSeconds();

      // Merge all tracks into a single sequential track
      juce::MidiMessageSequence mergedSequence;
      for (int i = 0; i < midiFile.getNumTracks(); ++i) {
        if (auto *track = midiFile.getTrack(i))
          mergedSequence.addSequence(*track, 0.0);
      }

      mergedSequence.updateMatchedPairs();

      // Re-store the sorted, merged sequence back into our player sequence
      midiSequence.clear();
      midiSequence.addSequence(mergedSequence, 0.0);

      durationSeconds = midiSequence.getEndTime();
      DBG("MidiPlayer: Loaded file. Tracks="
          << midiFile.getNumTracks()
          << ", Events=" << midiSequence.getNumEvents()
          << ", Duration=" << durationSeconds << "s");

      if (durationSeconds <= 0.0) {
        DBG("MidiPlayer: Warning - Duration is zero or negative! Forcing "
            "fallback duration.");
        durationSeconds = 60.0; // Fallback so it doesn't instantly stop
      }

      currentPositionSeconds = 0.0;
      nextEventIndex = 0;
      playing = false;

      return true;
    }
  }
  DBG("MidiPlayer: Error loading MIDI file from stream.");
  return false;
}

void MidiPlayer::setPosition(double timeInSeconds) {
  const juce::ScopedLock sl(lock);
  currentPositionSeconds = juce::jlimit(0.0, durationSeconds, timeInSeconds);

  // Find the next event index based on the new time
  nextEventIndex = midiSequence.getNextIndexAtTime(currentPositionSeconds);
}

double MidiPlayer::getPosition() const { return currentPositionSeconds; }

double MidiPlayer::getDuration() const { return durationSeconds; }

void MidiPlayer::play() { playing = true; }

void MidiPlayer::pause() { playing = false; }

void MidiPlayer::stop() {
  const juce::ScopedLock sl(lock);
  playing = false;
  currentPositionSeconds = 0.0;
  nextEventIndex = 0;
}

bool MidiPlayer::isPlaying() const { return playing; }

void MidiPlayer::getNextAudioBlock(juce::MidiBuffer &outputBuffer,
                                   int numSamples, double sampleRate) {
  outputBuffer.clear();

  if (!playing || midiSequence.getNumEvents() == 0)
    return;

  const juce::ScopedLock sl(lock);

  // Calculate how much time this buffer represents
  double bufferDurationSeconds = numSamples / sampleRate;
  double endPositionSeconds = currentPositionSeconds + bufferDurationSeconds;

  // Extract events within this time window
  while (nextEventIndex < midiSequence.getNumEvents()) {
    auto *event = midiSequence.getEventPointer(nextEventIndex);
    double eventTime = event->message.getTimeStamp();

    if (eventTime >= endPositionSeconds)
      break; // Event is beyond the current buffer

    // Convert event time to sample position within the buffer
    int sampleOffset = (int)((eventTime - currentPositionSeconds) * sampleRate);
    sampleOffset = juce::jlimit(0, numSamples - 1, sampleOffset);

    outputBuffer.addEvent(event->message, sampleOffset);
    // DBG("MidiPlayer: Event at " << eventTime << "s (offset " << sampleOffset
    // << ")");
    nextEventIndex++;
  }

  if (outputBuffer.getNumEvents() > 0)
    DBG("MidiPlayer: Pushed " << outputBuffer.getNumEvents()
                              << " events to buffer");

  currentPositionSeconds = endPositionSeconds;

  if (currentPositionSeconds >= durationSeconds) {
    DBG("MidiPlayer: Reached end of duration (" << durationSeconds
                                                << "s). Stopping playback.");
    playing = false; // Finished playback
  }
}

