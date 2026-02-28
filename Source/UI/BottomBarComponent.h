#pragma once

#include <JuceHeader.h>
#include <functional>

class BottomBarComponent : public juce::Component {
public:
  BottomBarComponent();
  ~BottomBarComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  // Custom Button class for precise icon control
  class IconButton : public juce::Button {
  public:
    IconButton(const juce::String &name, const juce::Path &p)
        : juce::Button(name), iconPath(p) {}

    void paintButton(juce::Graphics &g, bool isMouseOverButton,
                     bool isButtonDown) override {
      auto area = getLocalBounds().toFloat();

      // Draw circular button background
      g.setColour(juce::Colours::white.withAlpha(
          isButtonDown ? 0.8f : (isMouseOverButton ? 1.0f : 0.9f)));
      g.fillEllipse(area.reduced(2.0f));

      // Icon padding
      auto iconArea = area.reduced(13.0f);

      // Center and scale icon
      auto transform = iconPath.getTransformToScaleToFit(
          iconArea, true, juce::Justification::centred);

      // Use Dark Charcoal for icon to contrast with white button
      g.setColour(juce::Colour(0xff1A1A1B));
      if (isButtonDown)
        g.setColour(juce::Colours::black);

      g.fillPath(iconPath, transform);
    }

  private:
    juce::Path iconPath;
  };

  std::function<void()> onPlayClicked;
  std::function<void()> onStopClicked;
  std::function<void()> onNextClicked;
  std::function<void()> onSettingsClicked;
  std::function<void()> onMixerClicked;

private:
  std::unique_ptr<IconButton> playButton, stopButton, nextButton,
      settingsButton, mixerButton;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BottomBarComponent)
};
