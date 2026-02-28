#pragma once

#include "Core/Routing/MixerController.h"
#include "UI/MixerComponent.h"
#include <JuceHeader.h>

class SynthMixerWindow : public juce::DocumentWindow {
public:
  SynthMixerWindow(const juce::String &name, MixerController &mc)
      : DocumentWindow(
            name,
            juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(
                juce::ResizableWindow::backgroundColourId),
            DocumentWindow::allButtons) {
    setUsingNativeTitleBar(true);
    setContentOwned(new MixerComponent(mc), true);
    setResizable(true, false);
    setResizeLimits(600, 400, 2000, 1200);

    centreWithSize(1000, 600);
    setVisible(true);
  }

  void closeButtonPressed() override {
    // This is handled by the component owning this window
    setVisible(false);
  }

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthMixerWindow)
};

