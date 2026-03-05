#pragma once

#include "Audio/AudioGraphManager.h"
#include "Core/Routing/MixerController.h"
#include "UI/MixerComponent.h"
#include <JuceHeader.h>

class SynthMixerWindow : public juce::DocumentWindow {
public:
  SynthMixerWindow(const juce::String &name, MixerController &mc,
                   AudioGraphManager &agm)
      : DocumentWindow(
            name,
            juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(
                juce::ResizableWindow::backgroundColourId),
            DocumentWindow::allButtons) {
    setUsingNativeTitleBar(false);
    setContentOwned(new MixerComponent(mc, agm), true);
    setResizable(false, false);
    setAlwaysOnTop(true);

    centreWithSize(1350, 500);
  }

  MixerComponent &getMixerComponent() {
    return *dynamic_cast<MixerComponent *>(getContentComponent());
  }

  void closeButtonPressed() override {
    // This is handled by the component owning this window
    setVisible(false);
  }

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthMixerWindow)
};
