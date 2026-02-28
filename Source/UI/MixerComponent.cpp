#include "UI/MixerComponent.h"

//==============================================================================
// TrackStripComponent
//==============================================================================

TrackStripComponent::TrackStripComponent(MixerController &mc,
                                         InstrumentGroup group,
                                         const juce::String &name)
    : mixerController(mc), trackGroup(group), trackName(name) {

  mixerController.initializeTrack(group);

  gainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  gainSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  gainSlider.setRange(0.0, 4.0, 0.01);
  gainSlider.addListener(this);
  addAndMakeVisible(gainSlider);

  for (int i = 0; i < 3; ++i) {
    auxSends[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    auxSends[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    auxSends[i].setRange(0.0, 2.0, 0.01);
    auxSends[i].addListener(this);
    addAndMakeVisible(auxSends[i]);
  }

  fxButton.addListener(this);
  addAndMakeVisible(fxButton);

  volumeSlider.setSliderStyle(juce::Slider::LinearVertical);
  volumeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
  volumeSlider.setRange(0.0, 2.0, 0.01);
  volumeSlider.addListener(this);
  addAndMakeVisible(volumeSlider);

  panSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  panSlider.setRange(0.0, 1.0, 0.01);
  panSlider.addListener(this);
  addAndMakeVisible(panSlider);

  muteButton.setButtonText("M");
  muteButton.setClickingTogglesState(true);
  muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);
  muteButton.addListener(this);
  addAndMakeVisible(muteButton);

  soloButton.setButtonText("S");
  soloButton.setClickingTogglesState(true);
  soloButton.setColour(juce::TextButton::buttonOnColourId,
                       juce::Colours::yellow);
  soloButton.addListener(this);
  addAndMakeVisible(soloButton);

  addAndMakeVisible(vuMeter);

  updateStateFromController();
  startTimerHz(30);
}

TrackStripComponent::~TrackStripComponent() { stopTimer(); }

void TrackStripComponent::timerCallback() {
  vuMeter.setLevel(mixerController.getTrackLevel(trackGroup));
}

void TrackStripComponent::updateStateFromController() {
  gainSlider.setValue(mixerController.getTrackGain(trackGroup),
                      juce::dontSendNotification);
  for (int i = 0; i < 3; ++i)
    auxSends[i].setValue(mixerController.getTrackAuxSend(trackGroup, i),
                         juce::dontSendNotification);

  volumeSlider.setValue(mixerController.getTrackVolume(trackGroup),
                        juce::dontSendNotification);
  panSlider.setValue(mixerController.getTrackPan(trackGroup),
                     juce::dontSendNotification);
  muteButton.setToggleState(mixerController.isTrackMuted(trackGroup),
                            juce::dontSendNotification);
  soloButton.setToggleState(mixerController.isTrackSolo(trackGroup),
                            juce::dontSendNotification);
}

void TrackStripComponent::sliderValueChanged(juce::Slider *slider) {
  if (slider == &gainSlider) {
    mixerController.setTrackGain(trackGroup, (float)gainSlider.getValue());
  } else if (slider == &volumeSlider) {
    mixerController.setTrackVolume(trackGroup, (float)volumeSlider.getValue());
  } else if (slider == &panSlider) {
    mixerController.setTrackPan(trackGroup, (float)panSlider.getValue());
  } else {
    for (int i = 0; i < 3; ++i) {
      if (slider == &auxSends[i]) {
        mixerController.setTrackAuxSend(trackGroup, i,
                                        (float)auxSends[i].getValue());
        break;
      }
    }
  }
}

void TrackStripComponent::buttonClicked(juce::Button *button) {
  if (button == &muteButton) {
    mixerController.setTrackMute(trackGroup, muteButton.getToggleState());
  } else if (button == &soloButton) {
    mixerController.setTrackSolo(trackGroup, soloButton.getToggleState());
  } else if (button == &fxButton) {
    // TODO: Open FX window
  }
}

void TrackStripComponent::paint(juce::Graphics &g) {
  auto bgColour = juce::Colour(0xff1A1A1B);
  g.fillAll(bgColour);

  g.setColour(juce::Colours::white.withAlpha(0.1f));
  g.drawRect(getLocalBounds(), 1);

  g.setColour(juce::Colours::white);
  g.setFont(12.0f);
  g.drawText(trackName, 0, 2, getWidth(), 18, juce::Justification::centred,
             false);

  // Section labels
  g.setColour(juce::Colours::grey);
  g.setFont(10.0f);
  g.drawText("GAIN", 0, 20, getWidth(), 10, juce::Justification::centred,
             false);
}

void TrackStripComponent::resized() {
  auto area = getLocalBounds().reduced(4);
  area.removeFromTop(18); // Name label

  // Gain at top
  gainSlider.setBounds(area.removeFromTop(35).reduced(2));

  // FX Slot
  area.removeFromTop(5);
  fxButton.setBounds(area.removeFromTop(20).reduced(10, 0));

  // Aux Sends
  area.removeFromTop(5);
  auto auxArea = area.removeFromTop(80);
  for (int i = 0; i < 3; ++i) {
    auxSends[i].setBounds(auxArea.removeFromTop(25).reduced(15, 0));
  }

  // Bottom section: Pan, M/S, Fader + Meter
  area.removeFromTop(10);
  panSlider.setBounds(area.removeFromTop(35).reduced(2));

  auto msArea = area.removeFromTop(25);
  muteButton.setBounds(msArea.removeFromLeft(msArea.getWidth() / 2).reduced(2));
  soloButton.setBounds(msArea.reduced(2));

  area.removeFromTop(5);
  auto faderArea = area;

  // Overlapping VU and Fader
  vuMeter.setBounds(faderArea.removeFromRight(16));
  volumeSlider.setBounds(faderArea);
}

//==============================================================================
// MixerComponent
//==============================================================================

#include "Core/MidiHelper.h"

MixerComponent::MixerComponent(MixerController &mc)
    : mixerController(mc), content(mc, trackStrips),
      masterContent(mc, masterStrips) {
  // Define the order of appearance in the mixer UI
  const InstrumentGroup instrGroups[] = {
      // Melodic
      InstrumentGroup::Piano, InstrumentGroup::ChromaticPercussion,
      InstrumentGroup::ThaiRanad, InstrumentGroup::ThaiPongLang,
      InstrumentGroup::Organ, InstrumentGroup::Accordion,
      InstrumentGroup::ThaiKaen, InstrumentGroup::AcousticGuitarNylon,
      InstrumentGroup::AcousticGuitarSteel, InstrumentGroup::ElectricGuitarJazz,
      InstrumentGroup::ElectricGuitarClean, InstrumentGroup::OverdrivenGuitar,
      InstrumentGroup::DistortionGuitar, InstrumentGroup::Bass,
      InstrumentGroup::Strings, InstrumentGroup::Ensemble,
      InstrumentGroup::Brass, InstrumentGroup::Saxophone, InstrumentGroup::Reed,
      InstrumentGroup::Pipe, InstrumentGroup::ThaiKlui,
      InstrumentGroup::ThaiWote, InstrumentGroup::SynthLead,
      InstrumentGroup::SynthPad, InstrumentGroup::SynthEffects,
      InstrumentGroup::Ethnic, InstrumentGroup::ThaiPoengMang,
      InstrumentGroup::Percussive,

      // Drums
      InstrumentGroup::Kick, InstrumentGroup::Snare, InstrumentGroup::HandClap,
      InstrumentGroup::HiTom, InstrumentGroup::MidTom, InstrumentGroup::LowTom,
      InstrumentGroup::HiHat, InstrumentGroup::CrashCymbal,
      InstrumentGroup::RideCymbal, InstrumentGroup::Tambourine,
      InstrumentGroup::Cowbell, InstrumentGroup::Bongo, InstrumentGroup::Conga,
      InstrumentGroup::Timbale, InstrumentGroup::ThaiChing,
      InstrumentGroup::PercussionDrum,

      // VSTi
      InstrumentGroup::VSTi1, InstrumentGroup::VSTi2, InstrumentGroup::VSTi3,
      InstrumentGroup::VSTi4, InstrumentGroup::VSTi5, InstrumentGroup::VSTi6,
      InstrumentGroup::VSTi7, InstrumentGroup::VSTi8};

  const InstrumentGroup masterGroups[] = {
      // Vocals
      InstrumentGroup::VocalBus,
      // FX & Master
      InstrumentGroup::ReverbBus, InstrumentGroup::DelayBus,
      InstrumentGroup::ChorusBus, InstrumentGroup::MasterBus};

  for (const auto &grp : instrGroups) {
    trackStrips.add(new TrackStripComponent(mixerController, grp,
                                            MidiHelper::getThaiName(grp)));
  }
  for (auto *strip : trackStrips)
    content.addAndMakeVisible(strip);

  for (const auto &grp : masterGroups) {
    masterStrips.add(new TrackStripComponent(mixerController, grp,
                                             MidiHelper::getThaiName(grp)));
  }
  for (auto *strip : masterStrips)
    masterContent.addAndMakeVisible(strip);

  viewport.setViewedComponent(&content, false);
  viewport.setScrollBarsShown(false, true, false, true); // Horizontal scrollbar
  addAndMakeVisible(viewport);
  addAndMakeVisible(masterContent);
}

MixerComponent::~MixerComponent() {}

MixerComponent::ContentComponent::ContentComponent(
    MixerController &mc, juce::OwnedArray<TrackStripComponent> &strips)
    : trackStrips(strips) {}

void MixerComponent::ContentComponent::resized() {
  int stripWidth = 80;
  auto area = getLocalBounds();
  for (auto *strip : trackStrips) {
    strip->setBounds(area.removeFromLeft(stripWidth));
  }
}

void MixerComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff2d2d2d));
}

void MixerComponent::resized() {
  int stripWidth = 80;
  int masterWidth = masterStrips.size() * stripWidth;

  auto area = getLocalBounds();
  masterContent.setBounds(area.removeFromRight(masterWidth));
  viewport.setBounds(area);

  int totalWidth = trackStrips.size() * stripWidth;
  content.setSize(totalWidth,
                  viewport.getHeight() - (viewport.isHorizontalScrollBarShown()
                                              ? viewport.getScrollBarThickness()
                                              : 0));
  masterContent.resized();
}
