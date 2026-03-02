#include "UI/MixerComponent.h"
#include "Core/MidiHelper.h"

//==============================================================================
// Helpers
//==============================================================================
static float clampDb(float db) { return juce::jlimit(-99.0f, 6.0f, db); }

//==============================================================================
// ChannelStripComponent
//==============================================================================

ChannelStripComponent::ChannelStripComponent(MixerController &mc,
                                             AudioGraphManager &agm,
                                             InstrumentGroup group,
                                             const juce::String &name)
    : mixerController(mc), audioGraphManager(agm), trackGroup(group) {
  mixerController.initializeTrack(group);
  loadIcon();

  // Track name label
  nameLabel.setText(name, juce::dontSendNotification);
  nameLabel.setJustificationType(juce::Justification::centred);
  nameLabel.setFont(juce::Font(11.0f, juce::Font::bold));
  nameLabel.setColour(juce::Label::textColourId, juce::Colour(220, 220, 220));
  addAndMakeVisible(nameLabel);

  // Gain knob (top section)
  gainKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  gainKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  gainKnob.setRange(0.0, 2.0, 0.01);
  gainKnob.setValue(1.0, juce::dontSendNotification);
  gainKnob.addListener(this);
  addAndMakeVisible(gainKnob);

  // Mute / Solo
  muteButton.setButtonText("M");
  muteButton.setClickingTogglesState(true);
  muteButton.setColour(juce::TextButton::buttonOnColourId,
                       juce::Colours::red.withAlpha(0.85f));
  muteButton.addListener(this);
  addAndMakeVisible(muteButton);

  soloButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
  soloButton.addListener(this);
  addAndMakeVisible(soloButton);

  sf2ComboBox.setTooltip("Choose custom SoundFont for this track");
  sf2ComboBox.addItemList(mixerController.getAvailableSF2Names(), 1);
  sf2ComboBox.addListener(this);
  addAndMakeVisible(sf2ComboBox);

  // Output Selector
  outputSelector.addItem("SF2 (Default)", 1);
  for (int i = 1; i <= 8; ++i) {
    outputSelector.addItem("VSTi " + juce::String(i), i + 1);
  }
  outputSelector.setSelectedId(
      (int)mixerController.getTrackOutputDestination(trackGroup) + 1,
      juce::dontSendNotification);
  outputSelector.addListener(this);
  addAndMakeVisible(outputSelector);

  // 4 Insert FX Slots
  for (int i = 0; i < 4; ++i) {
    insertSlots[i].setButtonText("+");
    insertSlots[i].setTooltip("Insert FX " + juce::String(i + 1));
    addAndMakeVisible(insertSlots[i]);
  }

  // Meter
  addAndMakeVisible(meter);

  // Volume fader
  {
    auto dbRange = juce::NormalisableRange<double>(
        -99.0, 6.0,
        [](double, double, double p) {
          return (double)MixerFaderLAF::normToDb((float)p);
        },
        [](double, double, double v) {
          return (double)MixerFaderLAF::dbToNorm((float)v);
        });
    volumeFader.setNormalisableRange(dbRange);
    volumeFader.setSliderStyle(juce::Slider::LinearVertical);
    volumeFader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volumeFader.setLookAndFeel(&faderLAF);
    volumeFader.setValue(0.0, juce::dontSendNotification);
    volumeFader.addListener(this);
    addAndMakeVisible(volumeFader);
  }

  // Send knobs (3 small rotaries in a row)
  for (int i = 0; i < 3; ++i) {
    auxSends[i].setSliderStyle(juce::Slider::RotaryVerticalDrag);
    auxSends[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    auxSends[i].setRange(0.0, 1.0, 0.01);
    auxSends[i].setValue(0.0, juce::dontSendNotification);
    auxSends[i].addListener(this);
    addAndMakeVisible(auxSends[i]);
  }

  // Transpose hidden (not shown in UI, but still functional)
  transposeScale.setRange(-24, 24, 1);
  transposeScale.setValue(0, juce::dontSendNotification);
  transposeScale.addListener(this);

  updateStateFromController();
  setLookAndFeel(&mixerLAF);
  startTimerHz(60);
}

ChannelStripComponent::~ChannelStripComponent() { stopTimer(); }

void ChannelStripComponent::comboBoxChanged(juce::ComboBox *cb) {
  if (cb == &outputSelector) {
    auto dest = static_cast<OutputDestination>(cb->getSelectedId() - 1);
    mixerController.setTrackOutputDestination(trackGroup, dest);
    audioGraphManager.rebuildGraphRouting();
  } else if (cb == &sf2ComboBox) {
    if (sf2ComboBox.getSelectedId() == 1) {
      // Default SF2
      audioGraphManager.setTrackSoundFont(trackGroup, juce::File());
    } else {
      juce::String name = sf2ComboBox.getText();
      juce::File sf2File = mixerController.getSF2FileByName(name);
      if (sf2File.existsAsFile()) {
        audioGraphManager.setTrackSoundFont(trackGroup, sf2File);
      }
    }
  }
}

void ChannelStripComponent::timerCallback() {
  // Use left peak for the single meter representation
  meter.setLevel(mixerController.getTrackLevelLeft(trackGroup));
  meter.decayPeak();
}

void ChannelStripComponent::updateStateFromController() {
  gainKnob.setValue(mixerController.getTrackGain(trackGroup),
                    juce::dontSendNotification);
  transposeScale.setValue(mixerController.getTrackTranspose(trackGroup),
                          juce::dontSendNotification);

  float lv = mixerController.getTrackVolume(trackGroup);
  float db = lv <= 0.0f ? -99.0f : juce::Decibels::gainToDecibels(lv);
  volumeFader.setValue(clampDb(db), juce::dontSendNotification);

  // Use effectively muted for visual Mute button state
  muteButton.setToggleState(mixerController.isEffectivelyMuted(trackGroup),
                            juce::dontSendNotification);
  soloButton.setToggleState(mixerController.isTrackSolo(trackGroup),
                            juce::dontSendNotification);

  for (int i = 0; i < 3; ++i)
    auxSends[i].setValue(mixerController.getTrackAuxSend(trackGroup, i),
                         juce::dontSendNotification);

  // Update SF2 ComboBox
  sf2ComboBox.clear(juce::dontSendNotification);
  sf2ComboBox.addItemList(mixerController.getAvailableSF2Names(), 1);

  juce::String currentSf2 = mixerController.getTrackSF2Path(trackGroup);
  if (currentSf2.isEmpty()) {
    sf2ComboBox.setSelectedId(1, juce::dontSendNotification); // Default
  } else {
    juce::File f(currentSf2);
    sf2ComboBox.setText(f.getFileNameWithoutExtension(),
                        juce::dontSendNotification);
  }

  for (int i = 0; i < 4; ++i) {
    auto path = mixerController.getVstPluginPath(trackGroup, i);
    if (path.isNotEmpty()) {
      bool bypass = mixerController.getVstPluginBypass(trackGroup, i);
      insertSlots[i].setButtonText(
          bypass ? "[B] " + juce::File(path).getFileNameWithoutExtension()
                 : juce::File(path).getFileNameWithoutExtension());
    } else {
      insertSlots[i].setButtonText("+");
    }
  }
}

void ChannelStripComponent::setExpanded(bool expanded) {
  isExpanded = expanded;

  gainKnob.setVisible(expanded);
  for (int i = 0; i < 4; ++i)
    insertSlots[i].setVisible(expanded);
  for (int i = 0; i < 3; ++i)
    auxSends[i].setVisible(expanded);

  resized();
  repaint();
}

void ChannelStripComponent::sliderValueChanged(juce::Slider *s) {
  if (s == &gainKnob)
    mixerController.setTrackGain(trackGroup, (float)gainKnob.getValue());
  else if (s == &transposeScale)
    mixerController.setTrackTranspose(trackGroup,
                                      (int)transposeScale.getValue());
  else if (s == &volumeFader)
    mixerController.setTrackVolume(
        trackGroup, MixerFaderLAF::dbToGain((float)volumeFader.getValue()));
  else
    for (int i = 0; i < 3; ++i)
      if (s == &auxSends[i])
        mixerController.setTrackAuxSend(trackGroup, i,
                                        (float)auxSends[i].getValue());
}

void ChannelStripComponent::buttonClicked(juce::Button *b) {
  if (b == &muteButton)
    mixerController.setTrackMute(trackGroup, muteButton.getToggleState());
  if (b == &soloButton)
    mixerController.setTrackSolo(trackGroup, soloButton.getToggleState());

  for (int i = 0; i < 4; ++i) {
    if (b == &insertSlots[i]) {
      juce::PopupMenu menu;
      bool hasPlugin =
          mixerController.getVstPluginPath(trackGroup, i).isNotEmpty();

      if (hasPlugin) {
        menu.addItem(
            "Bypass", true, mixerController.getVstPluginBypass(trackGroup, i),
            [this, i]() {
              bool bypass = !mixerController.getVstPluginBypass(trackGroup, i);
              mixerController.setVstPluginBypass(trackGroup, i, bypass);
              audioGraphManager.rebuildGraphRouting();
              updateStateFromController();
            });
        menu.addItem("Remove Plugin", [this, i]() {
          audioGraphManager.removeVstFxPlugin(trackGroup, i);
          updateStateFromController();
        });
      } else {
        menu.addItem("Load VST...", [this, i]() {
          pluginChooser = std::make_unique<juce::FileChooser>(
              "Select VST3 Plugin", juce::File(), "*.vst3");
          pluginChooser->launchAsync(
              juce::FileBrowserComponent::openMode |
                  juce::FileBrowserComponent::canSelectFiles,
              [this, i](const juce::FileChooser &fc) {
                auto file = fc.getResult();
                if (file.existsAsFile()) {
                  if (audioGraphManager.loadVstFxPlugin(
                          trackGroup, i, file.getFullPathName())) {
                    updateStateFromController();
                  }
                }
              });
        });
      }
      menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(b));
      return;
    }
  }

  // Tell parent MixerComponent to refresh ALL visible strips
  if (auto *mixer = findParentComponentOfClass<MixerComponent>())
    mixer->updateAllStrips();
}

void ChannelStripComponent::loadIcon() {
  juce::String iconName;
  switch (trackGroup) {
  // Melodic
  case InstrumentGroup::Piano:
    iconName = "Piano.png";
    break;
  case InstrumentGroup::ChromaticPercussion:
    iconName = "ChromaticPercussion.png";
    break;
  case InstrumentGroup::ThaiRanad:
    iconName = "Ranad.png";
    break;
  case InstrumentGroup::ThaiPongLang:
    iconName = "Ponglang.png";
    break;
  case InstrumentGroup::Organ:
    iconName = "Organ.png";
    break;
  case InstrumentGroup::Accordion:
    iconName = "Accordion.png";
    break;
  case InstrumentGroup::ThaiKaen:
    iconName = "Khaen.png";
    break;
  case InstrumentGroup::AcousticGuitarNylon:
    iconName = "GuitarNyl.png";
    break;
  case InstrumentGroup::AcousticGuitarSteel:
    iconName = "GuitarAC.png";
    break;
  case InstrumentGroup::ElectricGuitarJazz:
    iconName = "Guitar3.png";
    break;
  case InstrumentGroup::ElectricGuitarClean:
    iconName = "Guitar1.png";
    break;
  case InstrumentGroup::OverdrivenGuitar:
    iconName = "Guitar2.png";
    break;
  case InstrumentGroup::DistortionGuitar:
    iconName = "Guitar4.png";
    break;
  case InstrumentGroup::Bass:
    iconName = "Bass.png";
    break;
  case InstrumentGroup::Strings:
    iconName = "Violins.png";
    break;
  case InstrumentGroup::Ensemble:
    iconName = "Violins.png";
    break;
  case InstrumentGroup::Brass:
    iconName = "Trombone.png";
    break;
  case InstrumentGroup::Saxophone:
    iconName = "Saxophone.png";
    break;
  case InstrumentGroup::Reed:
    iconName = "Oboe.png";
    break;
  case InstrumentGroup::Pipe:
    iconName = "Piccolo.png";
    break;
  case InstrumentGroup::ThaiKlui:
    iconName = "Klui.png";
    break;
  case InstrumentGroup::ThaiWote:
    iconName = "Wote.png";
    break;
  case InstrumentGroup::SynthLead:
    iconName = "Synthesizer1.png";
    break;
  case InstrumentGroup::SynthPad:
    iconName = "Synthesizer2.png";
    break;
  case InstrumentGroup::SynthEffects:
    iconName = "Synthesizer.png";
    break;
  case InstrumentGroup::Ethnic:
    iconName = "Ethnic.png";
    break;
  case InstrumentGroup::ThaiPoengMang:
    iconName = "Poengmang.png";
    break;
  case InstrumentGroup::Percussive:
    iconName = "TaikoDrum.png";
    break;

  // Drums
  case InstrumentGroup::Kick:
    iconName = "Kick.png";
    break;
  case InstrumentGroup::Snare:
    iconName = "Snare.png";
    break;
  case InstrumentGroup::HandClap:
    iconName = "HandClap.png";
    break;
  case InstrumentGroup::HiTom:
    iconName = "HiTom.png";
    break;
  case InstrumentGroup::MidTom:
    iconName = "MidTom.png";
    break;
  case InstrumentGroup::LowTom:
    iconName = "FloorTom.png";
    break;
  case InstrumentGroup::HiHat:
    iconName = "HiHat.png";
    break;
  case InstrumentGroup::CrashCymbal:
    iconName = "Crash.png";
    break;
  case InstrumentGroup::RideCymbal:
    iconName = "Ride.png";
    break;
  case InstrumentGroup::Tambourine:
    iconName = "Tambourine.png";
    break;
  case InstrumentGroup::Cowbell:
    iconName = "Cowbell.png";
    break;
  case InstrumentGroup::Bongo:
    iconName = "Bongo.png";
    break;
  case InstrumentGroup::Conga:
    iconName = "Conga.png";
    break;
  case InstrumentGroup::Timbale:
    iconName = "Timbale.png";
    break;
  case InstrumentGroup::ThaiChing:
    iconName = "Ching.png";
    break;
  case InstrumentGroup::PercussionDrum:
    iconName = "Percussion.png";
    break;

  default:
    iconName = "VST_Logo.png";
    break;
  }

  juce::File folder =
      juce::File::getSpecialLocation(juce::File::currentExecutableFile)
          .getParentDirectory();
  for (int i = 0; i < 6; ++i) {
    auto f = folder.getChildFile("Instruments").getChildFile(iconName);
    if (f.existsAsFile()) {
      instrumentIcon = juce::ImageCache::getFromFile(f);
      return;
    }
    folder = folder.getParentDirectory();
  }
}

void ChannelStripComponent::paint(juce::Graphics &g) {
  // Background gradient — dark charcoal
  auto r = getLocalBounds();
  juce::ColourGradient bg(juce::Colour(42, 42, 46), 0.0f, 0.0f,
                          juce::Colour(26, 26, 30), 0.0f, (float)r.getHeight(),
                          false);
  g.setGradientFill(bg);
  g.fillRect(r);

  // Right separator line
  g.setColour(juce::Colour(60, 60, 66));
  g.fillRect(r.getRight() - 1, r.getY(), 1, r.getHeight());

  if (isExpanded) {
    // GAIN label
    g.setFont(juce::Font(8.5f));
    g.setColour(juce::Colour(140, 140, 150));
    g.drawText("GAIN", 0, 4, getWidth(), 10, juce::Justification::centred);

    // Send labels (R1 R2 R3)
    g.setFont(juce::Font(7.5f));
    g.setColour(juce::Colour(100, 100, 115));
    g.drawText("INSERTS", 0, 84, getWidth(), 9, juce::Justification::centred);

    const int scrollbarPadding = 22;
    const int footerTotalH = 96 + scrollbarPadding;
    const int sendY = getHeight() - footerTotalH;
    const int sendW = getWidth() / 3;
    const char *sendLabels[] = {"FX1", "FX2", "FX3"};
    g.setFont(juce::Font(7.5f));
    g.setColour(juce::Colour(100, 140, 200));
    for (int i = 0; i < 3; ++i)
      g.drawText(sendLabels[i], i * sendW, sendY - 10, sendW, 10,
                 juce::Justification::centred);
  }
}

void ChannelStripComponent::resized() {
  const int W = getWidth();
  const int H = getHeight();
  int y = 0;

  if (isExpanded) {
    y += 12; // top padding + label
    gainKnob.setBounds(W / 2 - 16, y, 32, 32);
    y += 36;
    outputSelector.setBounds(4, y, W - 8, 18);
    y += 22;
  } else {
    y += 8;
  }

  y += 4;
  const int btnW = 22, btnH = 20;
  const int btnX = (W - btnW * 3 - 8) / 2;
  muteButton.setBounds(btnX, y, btnW, btnH);
  soloButton.setBounds(btnX + btnW + 4, y, btnW, btnH);
  sf2ComboBox.setBounds(btnX + (btnW + 4) * 2, y, btnW + 20,
                        btnH); // Slightly wider for combo
  y += btnH + 4;

  if (isExpanded) {
    const int slotH = 16;
    const int slotPad = 2;
    for (int i = 0; i < 4; ++i) {
      insertSlots[i].setBounds(4, y, W - 8, slotH);
      y += slotH + slotPad;
    }
    y += 2;
  }

  const int scrollbarPadding = 22;
  const int labelH = 20;
  const int iconH = 36;
  const int footerH = iconH + labelH + scrollbarPadding;
  int sendsH = isExpanded ? 44 : 0;
  int faderH = H - y - footerH - sendsH - 10;

  auto area = juce::Rectangle<int>(4, y, W - 8, faderH);
  const int meterW = 24;
  meter.setBounds(area.removeFromRight(meterW).reduced(0, 4));
  area.removeFromRight(4);
  volumeFader.setBounds(area);

  y += faderH + 10;

  if (isExpanded) {
    const int sendSz = 24;
    const int sendSpacing = (W - sendSz * 3) / 4;
    for (int i = 0; i < 3; ++i)
      auxSends[i].setBounds(sendSpacing + i * (sendSz + sendSpacing), y, sendSz,
                            sendSz);
    y += 36 + 4;
  }

  if (instrumentIcon.isValid()) {
    iconBounds = {0, y, W, iconH};
  }
  nameLabel.setBounds(0, y + iconH, W, labelH);
}

void ChannelStripComponent::paintOverChildren(juce::Graphics &g) {
  if (instrumentIcon.isValid()) {
    g.drawImageWithin(instrumentIcon, iconBounds.getX(), iconBounds.getY(),
                      iconBounds.getWidth(), iconBounds.getHeight(),
                      juce::RectanglePlacement::centred |
                          juce::RectanglePlacement::onlyReduceInSize);
  }
}

//==============================================================================
// FXStripComponent
//==============================================================================

FXStripComponent::FXStripComponent(MixerController &mc, AudioGraphManager &agm,
                                   InstrumentGroup group,
                                   const juce::String &name)
    : mixerController(mc), audioGraphManager(agm), fxGroup(group),
      fxName(name) {
  addAndMakeVisible(meter);

  for (int i = 0; i < 4; ++i) {
    insertSlots[i].setButtonText("+");
    insertSlots[i].setTooltip("Insert FX " + juce::String(i + 1));
    addAndMakeVisible(insertSlots[i]);
  }

  nameLabel.setText(fxName, juce::dontSendNotification);
  nameLabel.setJustificationType(juce::Justification::centred);
  nameLabel.setFont(juce::Font(11.0f, juce::Font::bold));
  nameLabel.setColour(juce::Label::textColourId, juce::Colour(220, 220, 220));
  addAndMakeVisible(nameLabel);

  returnVolumeFader.setSliderStyle(juce::Slider::LinearVertical);
  returnVolumeFader.setRange(-60.0, 6.0, 0.1);
  returnVolumeFader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  returnVolumeFader.setLookAndFeel(&faderLAF);
  returnVolumeFader.addListener(this);
  addAndMakeVisible(returnVolumeFader);

  setLookAndFeel(&mixerLAF);
  loadIcon();
  startTimerHz(60);
}

FXStripComponent::~FXStripComponent() { stopTimer(); }

void FXStripComponent::timerCallback() {
  // Use left peak for the single meter representation
  meter.setLevel(mixerController.getTrackLevelLeft(fxGroup));
  meter.decayPeak();
}

void FXStripComponent::sliderValueChanged(juce::Slider *s) {
  if (s == &returnVolumeFader)
    mixerController.setTrackVolume(
        fxGroup,
        juce::Decibels::decibelsToGain((float)returnVolumeFader.getValue()));
}

void FXStripComponent::setExpanded(bool expanded) {
  isExpanded = expanded;
  for (int i = 0; i < 4; ++i)
    insertSlots[i].setVisible(expanded);
  resized();
  repaint();
}

void FXStripComponent::paint(juce::Graphics &g) {
  auto r = getLocalBounds();
  // Standard charcoal gradient to match instrument channels
  juce::ColourGradient bg(juce::Colour(55, 55, 55), 0.0f, 0.0f,
                          juce::Colour(35, 35, 35), 0.0f, (float)r.getHeight(),
                          false);
  g.setGradientFill(bg);
  g.fillRect(r);

  // Right separator line
  g.setColour(juce::Colour(80, 80, 80));
  g.fillRect(r.getRight() - 1, r.getY(), 1, r.getHeight());

  // No footer badge, match instrument channel style
}

void FXStripComponent::resized() {
  const int W = getWidth();
  const int H = getHeight();
  int y = 0;

  if (isExpanded) {
    y += 12 + 36; // Skip gain
    y += 20 + 4;  // Skip M/S

    const int slotH = 16;
    const int slotPad = 2;
    for (int i = 0; i < 4; ++i) {
      insertSlots[i].setBounds(4, y, W - 8, slotH);
      y += slotH + slotPad;
    }
    y += 2;
  }

  const int scrollbarPadding = 22;
  const int iconH = 36;
  const int labelH = 20;
  const int footerH = iconH + labelH + scrollbarPadding;
  int sendsH = isExpanded ? 44 : 0;
  int faderH = H - y - footerH - sendsH - 10;

  auto area = juce::Rectangle<int>(4, y, W - 8, faderH);
  const int meterW = 24;
  meter.setBounds(area.removeFromRight(meterW).reduced(0, 4));
  area.removeFromRight(4);
  returnVolumeFader.setBounds(area);

  y += faderH + 10;
  if (isExpanded)
    y += 36 + 4; // auxiliary space

  if (instrumentIcon.isValid()) {
    const int iconH_footer = 36;
    const int labelH_footer = 20;
    auto footerArea =
        juce::Rectangle<int>(0, H - (iconH_footer + labelH_footer + 22), W,
                             iconH_footer + labelH_footer);
    iconBounds = footerArea.removeFromTop(iconH_footer);
    nameLabel.setBounds(footerArea);
  }
}

void FXStripComponent::paintOverChildren(juce::Graphics &g) {
  if (instrumentIcon.isValid()) {
    g.drawImageWithin(instrumentIcon, iconBounds.getX(), iconBounds.getY(),
                      iconBounds.getWidth(), iconBounds.getHeight(),
                      juce::RectanglePlacement::centred |
                          juce::RectanglePlacement::onlyReduceInSize);
  }
}

void FXStripComponent::loadIcon() {
  juce::String iconName = fxName + ".png";

  juce::File folder =
      juce::File::getSpecialLocation(juce::File::currentExecutableFile)
          .getParentDirectory();
  for (int i = 0; i < 6; ++i) {
    auto f = folder.getChildFile("Instruments").getChildFile(iconName);
    if (f.existsAsFile()) {
      instrumentIcon = juce::ImageCache::getFromFile(f);
      return;
    }
    folder = folder.getParentDirectory();
  }
}

//==============================================================================
// MasterStripComponent
//==============================================================================

MasterStripComponent::MasterStripComponent(MixerController &mc,
                                           AudioGraphManager &agm)
    : mixerController(mc), audioGraphManager(agm) {
  addAndMakeVisible(meterLeft);
  addAndMakeVisible(meterRight);

  for (int i = 0; i < 4; ++i) {
    insertSlots[i].setButtonText("+");
    insertSlots[i].setTooltip("Insert Master Bus " + juce::String(i + 1));
    addAndMakeVisible(insertSlots[i]);
  }

  nameLabel.setText("MASTER", juce::dontSendNotification);
  nameLabel.setJustificationType(juce::Justification::centred);
  nameLabel.setFont(juce::Font(11.0f, juce::Font::bold));
  nameLabel.setColour(juce::Label::textColourId, juce::Colour(220, 220, 220));
  addAndMakeVisible(nameLabel);

  masterFader.setSliderStyle(juce::Slider::LinearVertical);
  masterFader.setRange(-60.0, 6.0, 0.1);
  masterFader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  masterFader.setLookAndFeel(&faderLAF);
  masterFader.addListener(this);
  addAndMakeVisible(masterFader);

  setLookAndFeel(&mixerLAF);
  loadIcon();
  startTimerHz(60);
  repaint();
}

MasterStripComponent::~MasterStripComponent() { stopTimer(); }

void MasterStripComponent::timerCallback() {
  float peakL = mixerController.getTrackLevelLeft(InstrumentGroup::MasterBus);
  float peakR = mixerController.getTrackLevelRight(InstrumentGroup::MasterBus);

  meterLeft.setLevel(peakL);
  meterLeft.decayPeak();
  meterRight.setLevel(peakR);
  meterRight.decayPeak();
}

void MasterStripComponent::sliderValueChanged(juce::Slider *s) {
  if (s == &masterFader)
    mixerController.setTrackVolume(
        InstrumentGroup::MasterBus,
        juce::Decibels::decibelsToGain((float)masterFader.getValue()));
}

void MasterStripComponent::setExpanded(bool expanded) {
  isExpanded = expanded;
  for (int i = 0; i < 4; ++i)
    insertSlots[i].setVisible(expanded);
  resized();
  repaint();
}

void MasterStripComponent::paint(juce::Graphics &g) {
  auto r = getLocalBounds();
  // Deep gray/black gradient background
  juce::ColourGradient bg(juce::Colour(40, 40, 40), 0.0f, 0.0f,
                          juce::Colour(20, 20, 20), 0.0f, (float)r.getHeight(),
                          false);
  g.setGradientFill(bg);
  g.fillRect(r);

  // Right separator line
  g.setColour(juce::Colour(80, 80, 80));
  g.fillRect(r.getRight() - 1, r.getY(), 1, r.getHeight());

  const int iconH = 36;
  const int labelH = 20;
  const int scrollbarPadding = 22;
  auto footer = r.removeFromBottom(iconH + labelH + scrollbarPadding)
                    .withTrimmedBottom(scrollbarPadding);

  // Dark gray/black sleek footer
  g.setColour(juce::Colour(30, 30, 30));
  g.fillRoundedRectangle(footer.reduced(3, 2).toFloat(), 4.0f);

  g.setColour(juce::Colours::white);
  g.setFont(juce::Font(10.0f, juce::Font::bold));
  g.drawText("MASTER", footer.withTrimmedTop(iconH),
             juce::Justification::centred);
}

void MasterStripComponent::resized() {
  const int W = getWidth();
  const int H = getHeight();
  int y = 0;

  if (isExpanded) {
    y += 12 + 36; // Skip gain
    y += 20 + 4;  // Skip M/S

    const int slotH = 16;
    const int slotPad = 2;
    for (int i = 0; i < 4; ++i) {
      insertSlots[i].setBounds(4, y, W - 8, slotH);
      y += slotH + slotPad;
    }
    y += 2;
  }

  const int scrollbarPadding = 22;
  const int iconH = 36;
  const int labelH = 20;
  const int footerH = iconH + labelH + scrollbarPadding;
  int sendsH = isExpanded ? 44 : 0;
  int faderH = H - y - footerH - sendsH - 10;

  auto area = juce::Rectangle<int>(4, y, W - 8, faderH);
  auto meterArea = area.removeFromRight(52);
  meterLeft.setBounds(meterArea.removeFromLeft(24).reduced(0, 4));
  meterRight.setBounds(meterArea.removeFromRight(24).reduced(0, 4));
  area.removeFromRight(4);
  masterFader.setBounds(area);

  y += faderH + 10;
  if (isExpanded)
    y += 36 + 4; // auxiliary space

  if (instrumentIcon.isValid()) {
    const int iconH_footer = 36;
    const int labelH_footer = 20;
    auto footerArea =
        juce::Rectangle<int>(0, H - (iconH_footer + labelH_footer + 22), W,
                             iconH_footer + labelH_footer);
    iconBounds = footerArea.removeFromTop(iconH_footer);
    nameLabel.setBounds(footerArea);
  }
}

void MasterStripComponent::paintOverChildren(juce::Graphics &g) {
  if (instrumentIcon.isValid()) {
    g.drawImageWithin(instrumentIcon, iconBounds.getX(), iconBounds.getY(),
                      iconBounds.getWidth(), iconBounds.getHeight(),
                      juce::RectanglePlacement::centred |
                          juce::RectanglePlacement::onlyReduceInSize);
  }
}

void MasterStripComponent::loadIcon() {
  juce::String iconName = "Master.png";

  juce::File folder =
      juce::File::getSpecialLocation(juce::File::currentExecutableFile)
          .getParentDirectory();
  for (int i = 0; i < 6; ++i) {
    auto f = folder.getChildFile("Instruments").getChildFile(iconName);
    if (f.existsAsFile()) {
      instrumentIcon = juce::ImageCache::getFromFile(f);
      return;
    }
    folder = folder.getParentDirectory();
  }
}

//==============================================================================
// MixerComponent
//==============================================================================

void MixerComponent::toggleLayout() {
  isExpandedLayout = !isExpandedLayout;

  for (auto *strip : trackStrips)
    strip->setExpanded(isExpandedLayout);
  for (auto *strip : fxStrips)
    strip->setExpanded(isExpandedLayout);
  masterStrip.setExpanded(isExpandedLayout);

  if (auto *dw = findParentComponentOfClass<juce::DocumentWindow>()) {
    dw->centreWithSize(1500, isExpandedLayout ? 500 : 340);
  }
}

MixerComponent::MixerComponent(MixerController &mc, AudioGraphManager &agm)
    : mixerController(mc), audioGraphManager(agm), masterStrip(mc, agm),
      content(trackStrips), sideDocks(fxStrips, masterStrip) {
  addAndMakeVisible(header);

  // --- Melodic Instruments (0 - 27) ---
  for (int i = 0; i <= 27; ++i) {
    auto group = static_cast<InstrumentGroup>(i);
    auto *strip = trackStrips.add(
        new ChannelStripComponent(mixerController, audioGraphManager, group,
                                  MidiHelper::getThaiName(group)));
    content.addAndMakeVisible(strip);
  }

  // --- Drum Instruments (100 - 115) ---
  for (int i = 100; i <= 115; ++i) {
    auto group = static_cast<InstrumentGroup>(i);
    auto *strip = trackStrips.add(
        new ChannelStripComponent(mixerController, audioGraphManager, group,
                                  MidiHelper::getThaiName(group)));
    content.addAndMakeVisible(strip);
  }

  // --- Vocal Bus (149) ---
  {
    auto group = InstrumentGroup::VocalBus;
    auto *strip = trackStrips.add(
        new ChannelStripComponent(mixerController, audioGraphManager, group,
                                  MidiHelper::getThaiName(group)));
    content.addAndMakeVisible(strip);
  }

  // VSTi Buses removed: VSTi loading moved to Settings

  const std::pair<InstrumentGroup, juce::String> fxBuses[] = {
      {InstrumentGroup::ReverbBus, "FX1"},
      {InstrumentGroup::DelayBus, "FX2"},
      {InstrumentGroup::ChorusBus, "FX3"},
  };

  for (auto &fx : fxBuses) {
    auto *strip = fxStrips.add(new FXStripComponent(
        mixerController, audioGraphManager, fx.first, fx.second));
    sideDocks.addAndMakeVisible(strip);
  }

  header.layoutButton.onClick = [this] { toggleLayout(); };

  viewport.setViewedComponent(&content, false);
  viewport.setScrollBarsShown(false, true, false, true);
  viewport.setScrollBarThickness(14);
  viewport.setLookAndFeel(&getLookAndFeel());
  addAndMakeVisible(viewport);
  addAndMakeVisible(sideDocks);
}

MixerComponent::~MixerComponent() {}

void MixerComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff111113));
}

void MixerComponent::resized() {
  auto area = getLocalBounds();
  header.setBounds(area.removeFromTop(44));

  const int stripW = 76;
  const int masterW = 104;
  const int sideW = (int)fxStrips.size() * stripW + masterW; // 3 FX + 1 Master
  sideDocks.setBounds(area.removeFromRight(sideW));
  viewport.setBounds(area);

  content.setSize((int)trackStrips.size() * stripW, area.getHeight());
}

void MixerComponent::updateAllStrips() {
  for (auto *strip : trackStrips)
    strip->updateStateFromController();

  // FX strips don't show M/S buttons in this UI usually, but for consistency:
  // (FX strips were previously not updated, but we could add if needed)
}

//--

MixerComponent::ContentComponent::ContentComponent(
    juce::OwnedArray<ChannelStripComponent> &strips)
    : trackStrips(strips) {}

void MixerComponent::ContentComponent::resized() {
  auto area = getLocalBounds();
  for (auto *strip : trackStrips) {
    strip->setBounds(area.removeFromLeft(76));
  }
}

//--

MixerComponent::SideDocksComponent::SideDocksComponent(
    juce::OwnedArray<FXStripComponent> &fxStrips_,
    MasterStripComponent &master_)
    : fxStrips(fxStrips_), master(master_) {
  addAndMakeVisible(master);
  for (auto *fx : fxStrips)
    addAndMakeVisible(fx);
}

void MixerComponent::SideDocksComponent::resized() {
  auto area = getLocalBounds();
  for (auto *fx : fxStrips) {
    fx->setBounds(area.removeFromLeft(76));
  }
  master.setBounds(area.removeFromLeft(104));
}

//==============================================================================
// HeaderComponent
//==============================================================================

MixerComponent::HeaderComponent::HeaderComponent() {
  genreLabel.setText("Production Mode:", juce::dontSendNotification);
  genreLabel.setFont(juce::Font(12.0f, juce::Font::bold));
  genreLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  addAndMakeVisible(genreLabel);

  for (auto *btn :
       {&saveButton, &sfMapButton, &vstiSettingsButton, &layoutButton})
    addAndMakeVisible(btn);
}

void MixerComponent::HeaderComponent::paint(juce::Graphics &g) {
  auto r = getLocalBounds();
  g.setColour(juce::Colour(28, 28, 32));
  g.fillRect(r);

  g.setColour(juce::Colour(50, 50, 60));
  g.fillRect(r.getX(), r.getBottom() - 1, r.getWidth(), 1); // bottom border
}

void MixerComponent::HeaderComponent::resized() {
  auto area = getLocalBounds().reduced(12, 6);
  genreLabel.setBounds(area.removeFromLeft(160));
  area.removeFromLeft(10);

  layoutButton.setBounds(area.removeFromRight(72));
  area.removeFromRight(6);
  vstiSettingsButton.setBounds(area.removeFromRight(96));
  area.removeFromRight(6);
  sfMapButton.setBounds(area.removeFromRight(106));
  area.removeFromRight(6);
  saveButton.setBounds(area.removeFromRight(64));
}
