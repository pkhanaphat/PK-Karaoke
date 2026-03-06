#pragma once

#include "Audio/AudioGraphManager.h"
#include "Core/Routing/MixerController.h"
#include "UI/MixerComponent.h"
#include <JuceHeader.h>
#include <juce_opengl/juce_opengl.h>

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
    MixerComponent *mixer = new MixerComponent(mc, agm);
    int requiredW = mixer->getRequiredWidth();
    setContentOwned(mixer, true);
    setResizable(false, false);
    setAlwaysOnTop(true);

    openGLContext.attachTo(*this);

    centreWithSize(requiredW, 500);
  }

  ~SynthMixerWindow() override {
      openGLContext.detach();
  }

  MixerComponent &getMixerComponent() {
    return *dynamic_cast<MixerComponent *>(getContentComponent());
  }

  void closeButtonPressed() override {
    // This is handled by the component owning this window
    setVisible(false);
  }

private:
  juce::OpenGLContext openGLContext;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthMixerWindow)
};
