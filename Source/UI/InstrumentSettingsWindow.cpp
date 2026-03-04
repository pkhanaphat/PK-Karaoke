#include "UI/InstrumentSettingsWindow.h"
#include "Core/MidiHelper.h"

//==============================================================================
// InstrumentSettingsPanel
//==============================================================================

InstrumentSettingsPanel::InstrumentSettingsPanel(MixerController &mc,
                                                 AudioGraphManager &agm)
    : mixerController(mc), audioGraphManager(agm),
      tabs(juce::TabbedButtonBar::TabsAtTop), melodicTable("Melodic", nullptr),
      drumTable("Drum", nullptr) {

  setLookAndFeel(&laf);
  buildRows();

  melodicModel = std::make_unique<InstrumentTableModel>(mc, agm, melodicRows,
                                                        melodicTable);
  drumModel =
      std::make_unique<InstrumentTableModel>(mc, agm, drumRows, drumTable);

  melodicTable.setModel(melodicModel.get());
  drumTable.setModel(drumModel.get());

  setupTable(melodicTable, melodicModel.get());
  setupTable(drumTable, drumModel.get());

  headerLabel.setText("Instrument Output Settings", juce::dontSendNotification);
  headerLabel.setFont(juce::Font(17.0f, juce::Font::bold));
  headerLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  headerLabel.setJustificationType(juce::Justification::centredLeft);
  addAndMakeVisible(headerLabel);

  auto *melodicHolder = new juce::Component();
  melodicHolder->addAndMakeVisible(melodicTable);
  tabs.addTab("Melodic Instruments", juce::Colour(0xff252526), melodicHolder,
              true);

  auto *drumHolder = new juce::Component();
  drumHolder->addAndMakeVisible(drumTable);
  tabs.addTab("Drums / Percussion", juce::Colour(0xff252526), drumHolder, true);
  addAndMakeVisible(tabs);

  hintLabel.setText("Tip: Ctrl+Click or Shift+Click to select multiple rows",
                    juce::dontSendNotification);
  hintLabel.setFont(juce::Font(12.0f, juce::Font::italic));
  hintLabel.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
  hintLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(hintLabel);

  melodicModel->onSettingsChanged = [this] {
    if (onSettingsChanged)
      onSettingsChanged();
  };
  drumModel->onSettingsChanged = [this] {
    if (onSettingsChanged)
      onSettingsChanged();
  };

  setSize(560, 540);
}

InstrumentSettingsPanel::~InstrumentSettingsPanel() {
  melodicTable.setModel(nullptr);
  drumTable.setModel(nullptr);
  setLookAndFeel(nullptr);
}

void InstrumentSettingsPanel::buildRows() {
  melodicRows.clear();
  drumRows.clear();

  const InstrumentGroup melodic[] = {InstrumentGroup::Piano,
                                     InstrumentGroup::ChromaticPercussion,
                                     InstrumentGroup::ThaiRanad,
                                     InstrumentGroup::ThaiPongLang,
                                     InstrumentGroup::Organ,
                                     InstrumentGroup::Accordion,
                                     InstrumentGroup::ThaiKaen,
                                     InstrumentGroup::AcousticGuitarNylon,
                                     InstrumentGroup::AcousticGuitarSteel,
                                     InstrumentGroup::ElectricGuitarJazz,
                                     InstrumentGroup::ElectricGuitarClean,
                                     InstrumentGroup::OverdrivenGuitar,
                                     InstrumentGroup::DistortionGuitar,
                                     InstrumentGroup::Bass,
                                     InstrumentGroup::Strings,
                                     InstrumentGroup::Ensemble,
                                     InstrumentGroup::Brass,
                                     InstrumentGroup::Saxophone,
                                     InstrumentGroup::Reed,
                                     InstrumentGroup::Pipe,
                                     InstrumentGroup::ThaiKlui,
                                     InstrumentGroup::ThaiWote,
                                     InstrumentGroup::SynthLead,
                                     InstrumentGroup::SynthPad,
                                     InstrumentGroup::SynthEffects,
                                     InstrumentGroup::Ethnic,
                                     InstrumentGroup::ThaiPoengMang,
                                     InstrumentGroup::Percussive,
                                     InstrumentGroup::VocalBus};
  for (auto g : melodic)
    melodicRows.add({g, MidiHelper::getThaiName(g)});

  const InstrumentGroup drums[] = {
      InstrumentGroup::Kick,        InstrumentGroup::Snare,
      InstrumentGroup::HiTom,       InstrumentGroup::MidTom,
      InstrumentGroup::LowTom,      InstrumentGroup::HiHat,
      InstrumentGroup::CrashCymbal, InstrumentGroup::RideCymbal,
      InstrumentGroup::Cowbell,     InstrumentGroup::Tambourine,
      InstrumentGroup::HandClap,    InstrumentGroup::Bongo,
      InstrumentGroup::Conga,       InstrumentGroup::Timbale,
      InstrumentGroup::ThaiChing,   InstrumentGroup::PercussionDrum};
  for (auto g : drums)
    drumRows.add({g, MidiHelper::getThaiName(g)});
}

void InstrumentSettingsPanel::setupTable(juce::TableListBox &table,
                                         juce::TableListBoxModel * /*model*/) {
  auto &hdr = table.getHeader();
  hdr.addColumn("Instrument", 1, 220, 150, 350,
                juce::TableHeaderComponent::notResizable);
  hdr.addColumn("Output", 2, 200, 150, 300,
                juce::TableHeaderComponent::notResizable);

  table.setRowHeight(30);
  table.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff252526));
  table.setColour(juce::ListBox::outlineColourId, juce::Colour(0xff444444));
  table.setOutlineThickness(1);
  table.setMultipleSelectionEnabled(true);
}

void InstrumentSettingsPanel::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff1e1e1e));
}

void InstrumentSettingsPanel::resized() {
  auto area = getLocalBounds().reduced(16);

  headerLabel.setBounds(area.removeFromTop(36));
  area.removeFromTop(8);

  hintLabel.setBounds(area.removeFromBottom(24));
  area.removeFromBottom(4);

  tabs.setBounds(area);

  for (int i = 0; i < tabs.getNumTabs(); ++i) {
    if (auto *holder = tabs.getTabContentComponent(i)) {
      if (auto *table = holder->getChildComponent(0))
        table->setBounds(holder->getLocalBounds());
    }
  }
}

//==============================================================================
// InstrumentSettingsWindow
//==============================================================================

InstrumentSettingsWindow::InstrumentSettingsWindow(
    MixerController &mc, AudioGraphManager &agm,
    juce::Component * /*parentForPosition*/)
    : juce::DocumentWindow(
          "Instrument Settings",
          juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(
              juce::ResizableWindow::backgroundColourId),
          juce::DocumentWindow::closeButton) {

  setUsingNativeTitleBar(false);
  panel = std::make_unique<InstrumentSettingsPanel>(mc, agm);
  setContentOwned(panel.get(), false);
  setResizable(true, false);
  setAlwaysOnTop(true);

  panel->onSettingsChanged = [this] {
    if (onSettingsChanged)
      onSettingsChanged();
  };

  panel->onRightClickBackground = [this] {
    if (onRightClickBackground)
      onRightClickBackground();
  };

  centreWithSize(560, 540);
  setVisible(true);
}

InstrumentSettingsWindow::~InstrumentSettingsWindow() { panel.release(); }

void InstrumentSettingsWindow::closeButtonPressed() { setVisible(false); }
