#pragma once

#include <JuceHeader.h>
#include <vector>

struct SongLyrics
{
    std::vector<juce::String> lines;
    std::vector<double> cursors; // Seconds
};

class NcnParser
{
public:
    NcnParser();
    ~NcnParser() = default;

    /**
     * Parses the .lyr and .cur files.
     * @param lyrFile Path to the .lyr file
     * @param curFile Path to the .cur file
     * @param midiResolution The PPQ (Pulses Per Quarter Note) of the MIDI file
     * @return A SongLyrics object containing the parsed lyrics and synchronized cursors.
     */
    SongLyrics parse (const juce::File& lyrFile, const juce::File& curFile, int midiResolution);

private:
    juce::String convertTis620ToUtf8 (const juce::MemoryBlock& data);
};
