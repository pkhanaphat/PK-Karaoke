#pragma once

#include <JuceHeader.h>

class MidiPlayer {
public:
  MidiPlayer();
  ~MidiPlayer() = default;

  bool loadMidiFile(const juce::File &file);

  // Sets the playback position in seconds
  void setPosition(double timeInSeconds);

  // Gets the current playback position in seconds
  double getPosition() const;

  // Gets the current playback position in MIDI ticks
  int getPositionTicks() const;

  // Gets the total duration in seconds
  double getDuration() const;

  // Call this within an audio processing block to get the events for the
  // current buffer
  void getNextAudioBlock(juce::MidiBuffer &outputBuffer, int numSamples,
                         double sampleRate);

  void play();
  void pause();
  void stop();
  bool isPlaying() const;
  bool hasFinishedPlayback() const { return reachedEnd; }
  void clearFinishedPlayback() { reachedEnd = false; }
  int getNumEvents() const { return midiSequence.getNumEvents(); }

private:
  juce::MidiMessageSequence
      midiSequence; // Kept in seconds for audio buffer rendering
  juce::MidiMessageSequence
      midiSequenceTicks; // Kept in ticks for position tracking
  double currentPositionSeconds = 0.0;
  double durationSeconds = 0.0;
  int durationTicks = 0;
  short midiResolution = 0;
  bool playing = false;
  bool reachedEnd = false;

  // For tracking which events have been played
  int nextEventIndex = 0;

  juce::CriticalSection lock;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiPlayer)
};
