#include "MixerComponent.h"

//==============================================================================
// TrackStripComponent
//==============================================================================

TrackStripComponent::TrackStripComponent (MixerController& mc, InstrumentGroup group, const juce::String& name)
    : mixerController (mc), trackGroup (group), trackName (name)
{
    volumeSlider.setSliderStyle (juce::Slider::LinearVertical);
    volumeSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 20);
    volumeSlider.setRange (0.0, 2.0, 0.01);
    volumeSlider.setValue (1.0);
    volumeSlider.addListener (this);
    addAndMakeVisible (volumeSlider);

    panSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    panSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    panSlider.setRange (0.0, 1.0, 0.01);
    panSlider.setValue (0.5);
    panSlider.addListener (this);
    addAndMakeVisible (panSlider);

    muteButton.setButtonText ("M");
    muteButton.setClickingTogglesState (true);
    muteButton.setColour (juce::TextButton::buttonOnColourId, juce::Colours::red);
    muteButton.addListener (this);
    addAndMakeVisible (muteButton);

    soloButton.setButtonText ("S");
    soloButton.setClickingTogglesState (true);
    soloButton.setColour (juce::TextButton::buttonOnColourId, juce::Colours::yellow);
    soloButton.addListener (this);
    addAndMakeVisible (soloButton);

    updateStateFromController();
}

TrackStripComponent::~TrackStripComponent()
{
}

void TrackStripComponent::updateStateFromController()
{
    volumeSlider.setValue (mixerController.getTrackVolume (trackGroup), juce::dontSendNotification);
    panSlider.setValue (mixerController.getTrackPan (trackGroup), juce::dontSendNotification);
    muteButton.setToggleState (mixerController.isTrackMuted (trackGroup), juce::dontSendNotification);
    soloButton.setToggleState (mixerController.isTrackSolo (trackGroup), juce::dontSendNotification);
}

void TrackStripComponent::sliderValueChanged (juce::Slider* slider)
{
    if (slider == &volumeSlider)
    {
        mixerController.setTrackVolume (trackGroup, (float)volumeSlider.getValue());
    }
    else if (slider == &panSlider)
    {
        mixerController.setTrackPan (trackGroup, (float)panSlider.getValue());
    }
}

void TrackStripComponent::buttonClicked (juce::Button* button)
{
    if (button == &muteButton)
    {
        mixerController.setTrackMute (trackGroup, muteButton.getToggleState());
    }
    else if (button == &soloButton)
    {
        mixerController.setTrackSolo (trackGroup, soloButton.getToggleState());
    }
}

void TrackStripComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey.darker());
    g.setColour (juce::Colours::grey);
    g.drawRect (getLocalBounds(), 1);

    g.setColour (juce::Colours::white);
    g.setFont (14.0f);
    g.drawText (trackName, 0, 5, getWidth(), 20, juce::Justification::centred, false);
}

void TrackStripComponent::resized()
{
    auto area = getLocalBounds().reduced (5);
    area.removeFromTop (20); // Name label space

    panSlider.setBounds (area.removeFromTop (40));

    auto buttonArea = area.removeFromTop (25);
    muteButton.setBounds (buttonArea.removeFromLeft (area.getWidth() / 2).reduced (2));
    soloButton.setBounds (buttonArea.reduced (2));

    area.removeFromTop (5); // Gap
    volumeSlider.setBounds (area);
}

//==============================================================================
// MixerComponent
//==============================================================================

#include "MidiHelper.h"

MixerComponent::MixerComponent (MixerController& mc) 
    : mixerController (mc), 
      content (mc, trackStrips)
{
    // Define the order of appearance in the mixer UI
    const InstrumentGroup groups[] = {
        // Melodic
        InstrumentGroup::Piano, InstrumentGroup::ChromaticPercussion, InstrumentGroup::ThaiRanad,
        InstrumentGroup::ThaiPongLang, InstrumentGroup::Organ, InstrumentGroup::Accordion,
        InstrumentGroup::ThaiKaen, InstrumentGroup::AcousticGuitarNylon, InstrumentGroup::AcousticGuitarSteel,
        InstrumentGroup::ElectricGuitarJazz, InstrumentGroup::ElectricGuitarClean, InstrumentGroup::OverdrivenGuitar,
        InstrumentGroup::DistortionGuitar, InstrumentGroup::Bass, InstrumentGroup::Strings,
        InstrumentGroup::Ensemble, InstrumentGroup::Brass, InstrumentGroup::Saxophone,
        InstrumentGroup::Reed, InstrumentGroup::Pipe, InstrumentGroup::ThaiKlui,
        InstrumentGroup::ThaiWote, InstrumentGroup::SynthLead, InstrumentGroup::SynthPad,
        InstrumentGroup::SynthEffects, InstrumentGroup::Ethnic, InstrumentGroup::ThaiPoengMang,
        InstrumentGroup::Percussive,

        // Drums
        InstrumentGroup::Kick, InstrumentGroup::Snare, InstrumentGroup::HandClap,
        InstrumentGroup::HiTom, InstrumentGroup::MidTom, InstrumentGroup::LowTom,
        InstrumentGroup::HiHat, InstrumentGroup::CrashCymbal, InstrumentGroup::RideCymbal,
        InstrumentGroup::Tambourine, InstrumentGroup::Cowbell, InstrumentGroup::Bongo,
        InstrumentGroup::Conga, InstrumentGroup::Timbale, InstrumentGroup::ThaiChing,
        InstrumentGroup::PercussionDrum,

        // Vocals
        InstrumentGroup::VocalBus,

        // VSTi
        InstrumentGroup::VSTi1, InstrumentGroup::VSTi2, InstrumentGroup::VSTi3, InstrumentGroup::VSTi4,
        InstrumentGroup::VSTi5, InstrumentGroup::VSTi6, InstrumentGroup::VSTi7, InstrumentGroup::VSTi8,

        // FX & Master
        InstrumentGroup::ReverbBus, InstrumentGroup::DelayBus, InstrumentGroup::ChorusBus, InstrumentGroup::MasterBus
    };

    for (const auto& grp : groups)
    {
        trackStrips.add (new TrackStripComponent (mixerController, grp, MidiHelper::getThaiName(grp)));
    }

    for (auto* strip : trackStrips)
        content.addAndMakeVisible (strip);

    viewport.setViewedComponent (&content, false);
    viewport.setScrollBarsShown (false, true, false, true); // Horizontal scrollbar
    addAndMakeVisible (viewport);
}

MixerComponent::~MixerComponent()
{
}

MixerComponent::ContentComponent::ContentComponent (MixerController& mc, juce::OwnedArray<TrackStripComponent>& strips)
    : trackStrips (strips)
{
}

void MixerComponent::ContentComponent::resized()
{
    int stripWidth = 80;
    auto area = getLocalBounds();
    for (auto* strip : trackStrips)
    {
        strip->setBounds (area.removeFromLeft (stripWidth));
    }
}

void MixerComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff2d2d2d));
}

void MixerComponent::resized()
{
    viewport.setBounds (getLocalBounds());
    
    int totalWidth = trackStrips.size() * 80;
    content.setSize (totalWidth, viewport.getHeight() - (viewport.isHorizontalScrollBarShown() ? viewport.getScrollBarThickness() : 0));
}
