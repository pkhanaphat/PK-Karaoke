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
    setUsingNativeTitleBar(false);
    setContentOwned(new MixerComponent(mc), true);
    setResizable(false, false);
    setAlwaysOnTop(true);

    centreWithSize(1380, 500);
  }

  void closeButtonPressed() override {
    // This is handled by the component owning this window
    setVisible(false);
  }

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthMixerWindow)
};
