#pragma once

#include <JuceHeader.h>
#include "MixerController.h"

class TrackStripComponent : public juce::Component, public juce::Slider::Listener, public juce::Button::Listener
{
public:
    TrackStripComponent (MixerController& mc, InstrumentGroup group, const juce::String& name);
    ~TrackStripComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void sliderValueChanged (juce::Slider* slider) override;
    void buttonClicked (juce::Button* button) override;

    void updateStateFromController();

private:
    MixerController& mixerController;
    InstrumentGroup trackGroup;
    juce::String trackName;

    juce::Slider volumeSlider;
    juce::Slider panSlider;
    juce::TextButton muteButton;
    juce::TextButton soloButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackStripComponent)
};

class MixerComponent : public juce::Component
{
public:
    MixerComponent (MixerController& mc);
    ~MixerComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    MixerController& mixerController;
    
    struct ContentComponent : public juce::Component
    {
        ContentComponent (MixerController& mc, juce::OwnedArray<TrackStripComponent>& strips);
        void resized() override;
        juce::OwnedArray<TrackStripComponent>& trackStrips;
    };

    juce::OwnedArray<TrackStripComponent> trackStrips;
    ContentComponent content;
    juce::Viewport viewport;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerComponent)
};
