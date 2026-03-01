#pragma once

#include <JuceHeader.h>
#include <functional>

class BottomBarComponent : public juce::Component {
public:
  BottomBarComponent();
  ~BottomBarComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;
  void updateSeekPosition(double normalizedPosition);

  // Custom Button class for precise icon control
  class IconButton : public juce::Button {
  public:
    IconButton(const juce::String &name, const juce::Path &p)
        : juce::Button(name), iconPath(p) {}

    void paintButton(juce::Graphics &g, bool isMouseOverButton,
                     bool isButtonDown) override {
      auto area = getLocalBounds().toFloat();

      // No background, just the icon itself.
      // Use less reduction since there's no background circle padding needed.
      auto iconArea = area.reduced(2.0f);

      auto transform = iconPath.getTransformToScaleToFit(
          iconArea, true, juce::Justification::centred);

      // Icon color: White normally, slightly dimmed on hover, more dimmed on
      // press
      if (isButtonDown)
        g.setColour(juce::Colours::white.withAlpha(0.5f));
      else if (isMouseOverButton)
        g.setColour(juce::Colours::white.withAlpha(0.8f));
      else
        g.setColour(juce::Colours::white);

      g.fillPath(iconPath, transform);
    }

  private:
    juce::Path iconPath;
  };

  std::function<void()> onPlayClicked;
  std::function<void()> onStopClicked;
  std::function<void()> onNextClicked;
  std::function<void(double)>
      onSeekChanged; // Callback taking the new normalized position (0.0 to 1.0)
  std::function<void()> onMixerClicked;

private:
  std::unique_ptr<IconButton> playButton, stopButton, nextButton, mixerButton;
  juce::Slider seekSlider;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BottomBarComponent)
};
