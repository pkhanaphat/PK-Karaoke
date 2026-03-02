#pragma once

#include "Core/MidiPlayer.h"
#include "Core/NcnParser.h"
#include <BinaryData.h>
#include <JuceHeader.h>

class LyricsView : public juce::Component, public juce::Timer {
public:
  LyricsView();
  ~LyricsView() override;

  void paint(juce::Graphics &) override;
  void resized() override;

  void setLyrics(const SongLyrics &newLyrics);
  void attachToPlayer(MidiPlayer *player);
  void setCustomBackgroundImage(const juce::Image &newImage);

  // Call this repeatedly or use a timer to ask the player for position
  void timerCallback() override;

private:
  SongLyrics currentLyrics;
  MidiPlayer *attachedPlayer = nullptr;

  // For timing tracking
  int currentPositionTicks = 0;

  // UI layout params
  juce::Font lyricsFont;
  int currentCursorIndex = 0; // Where we are in the lyrics.cursors array
  juce::Image backgroundImage;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LyricsView)
};
