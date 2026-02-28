#pragma once

#include <JuceHeader.h>
#include <vector>
#include "MixerController.h"

class MidiHelper
{
public:
    static std::vector<InstrumentGroup> getDrumInstrumentTypes();
    static InstrumentGroup getInstrumentDrumType (int drumNote);
    static InstrumentGroup getInstrumentType (int instNumber);
    
    // For UI display
    static juce::String getThaiName (InstrumentGroup type);
};
