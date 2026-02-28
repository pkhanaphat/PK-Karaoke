#pragma once

#include <JuceHeader.h>
#include "NcnParser.h"
#include "MidiPlayer.h"

class LyricsView : public juce::Component, public juce::Timer
{
public:
    LyricsView();
    ~LyricsView() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setLyrics (const SongLyrics& newLyrics);
    void attachToPlayer (MidiPlayer* player);

    // Call this repeatedly or use a timer to ask the player for position
    void timerCallback() override;

private:
    SongLyrics currentLyrics;
    MidiPlayer* attachedPlayer = nullptr;
    
    // For timing tracking
    double currentPositionSeconds = 0.0;
    
    // UI layout params
    juce::Font lyricsFont;
    int currentCursorIndex = 0; // Where we are in the lyrics.cursors array

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LyricsView)
};
