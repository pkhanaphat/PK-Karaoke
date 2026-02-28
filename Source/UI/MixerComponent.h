#pragma once

#include "Core/Routing/MixerController.h"
#include <JuceHeader.h>

class TrackStripComponent : public juce::Component,
                            public juce::Slider::Listener,
                            public juce::Button::Listener,
                            public juce::Timer {
public:
  TrackStripComponent(MixerController &mc, InstrumentGroup group,
                      const juce::String &name);
  ~TrackStripComponent() override;

  void paint(juce::Graphics &) override;
  void resized() override;

  void sliderValueChanged(juce::Slider *slider) override;
  void buttonClicked(juce::Button *button) override;

  void updateStateFromController();
  void timerCallback() override;

private:
  MixerController &mixerController;
  InstrumentGroup trackGroup;
  juce::String trackName;

  juce::Slider volumeSlider;
  juce::Slider panSlider;
  juce::Slider gainSlider;
  juce::Slider auxSends[3];

  class FXSlotButton : public juce::Button {
  public:
    FXSlotButton(const juce::String &name) : juce::Button(name) {
      setClickingTogglesState(true);
    }
    void paintButton(juce::Graphics &g, bool p, bool h) override {
      auto area = getLocalBounds().toFloat();
      g.setColour(juce::Colour(0xff111111));
      g.fillRect(area);
      g.setColour(juce::Colour(0xff333333));
      g.drawRect(area, 1.0f);

      auto dotArea = area.removeFromLeft(14).reduced(2);
      g.setColour(juce::Colour(0xff222222));
      g.drawRoundedRectangle(dotArea, 2.0f, 1.0f);

      if (getToggleState()) {
        g.setColour(juce::Colours::white);
        g.fillEllipse(dotArea.reduced(3));
      }

      g.setColour(juce::Colour(0xff888888));
      g.setFont(8.0f);
      g.drawText(getName(), area, juce::Justification::centred);
    }
  };

  FXSlotButton fxButton{"FX SLOT"};
  juce::TextButton muteButton;
  juce::TextButton soloButton;

#include "UI/VUMeterComponent.h"
  VUMeterComponent vuMeter;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackStripComponent)
};

class MixerComponent : public juce::Component {
public:
  MixerComponent(MixerController &mc);
  ~MixerComponent() override;

  void paint(juce::Graphics &) override;
  void resized() override;

private:
  MixerController &mixerController;

  struct ContentComponent : public juce::Component {
    ContentComponent(MixerController &mc,
                     juce::OwnedArray<TrackStripComponent> &strips);
    void resized() override;
    juce::OwnedArray<TrackStripComponent> &trackStrips;
  };

  juce::OwnedArray<TrackStripComponent> trackStrips;
  juce::OwnedArray<TrackStripComponent> masterStrips;
  ContentComponent content;
  ContentComponent masterContent;
  juce::Viewport viewport;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerComponent)
};
