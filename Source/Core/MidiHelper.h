#pragma once

#include "Core/Routing/MixerController.h"
#include <JuceHeader.h>
#include <vector>


class MidiHelper {
public:
  static std::vector<InstrumentGroup> getDrumInstrumentTypes();
  static InstrumentGroup getInstrumentDrumType(int drumNote);
  static InstrumentGroup getInstrumentType(int instNumber);

  // For UI display
  static juce::String getThaiName(InstrumentGroup type);
};

