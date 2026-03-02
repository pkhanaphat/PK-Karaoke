#include "UI/SettingsComponent.h"

DatabaseSettingsPanel::DatabaseSettingsPanel() : progressBar(progress) {
  addAndMakeVisible(ncnLabel);
  addAndMakeVisible(ncnPathEditor);
  addAndMakeVisible(ncnBrowseButton);

  addAndMakeVisible(pkmLabel);
  addAndMakeVisible(pkmPathEditor);
  addAndMakeVisible(pkmBrowseButton);

  addAndMakeVisible(progressLabel);
  addAndMakeVisible(progressBar);

  addAndMakeVisible(totalSongsLabel);
  addAndMakeVisible(buildDbButton);

  ncnPathEditor.setReadOnly(true);
  pkmPathEditor.setReadOnly(true);

  // Set Thai text in constructor to ensure u8 literal is used in .cpp context
  ncnLabel.setText(u8"NCN :", juce::dontSendNotification);
  pkmLabel.setText(u8"PKM :", juce::dontSendNotification);
  progressLabel.setText(u8"ความคืบหน้า :", juce::dontSendNotification);
  buildDbButton.setButtonText(u8"ปรับปรุง");
  totalSongsLabel.setText(u8"จำนวนเพลงทั้งหมด : 0 เพลง",
                          juce::dontSendNotification);
}

void DatabaseSettingsPanel::paint(juce::Graphics &g) {
  g.fillAll(juce::Colours::transparentBlack);

  g.setColour(juce::Colours::white);
  g.setFont(18.0f);
  g.drawText(u8"ที่เก็บเพลง", 10, 10, 200, 30, juce::Justification::centredLeft,
             true);

  g.drawText(u8"ปรับปรุงฐานข้อมูลเพลง", 10, 110, 200, 30,
             juce::Justification::centredLeft, true);
}

void DatabaseSettingsPanel::resized() {
  auto area = getLocalBounds().reduced(10);
  area.removeFromTop(40);

  auto ncnBounds = area.removeFromTop(30);
  ncnLabel.setBounds(ncnBounds.removeFromLeft(60));
  ncnBrowseButton.setBounds(ncnBounds.removeFromRight(30));
  ncnBounds.removeFromRight(10);
  ncnPathEditor.setBounds(ncnBounds);

  area.removeFromTop(10);

  auto pkmBounds = area.removeFromTop(30);
  pkmLabel.setBounds(pkmBounds.removeFromLeft(60));
  pkmBrowseButton.setBounds(pkmBounds.removeFromRight(30));
  pkmBounds.removeFromRight(10);
  pkmPathEditor.setBounds(pkmBounds);

  area.removeFromTop(40);

  auto progressBounds = area.removeFromTop(20);
  progressLabel.setBounds(progressBounds.removeFromLeft(120));
  progressBar.setBounds(progressBounds);

  area.removeFromTop(10);

  totalSongsLabel.setBounds(area.removeFromTop(30));

  area.removeFromTop(10);
  buildDbButton.setBounds(
      area.removeFromTop(40).withSizeKeepingCentre(200, 40));
}

void DatabaseSettingsPanel::updateProgress(double newProgress) {
  progress = newProgress;
  progressBar.setPercentageDisplay(true);
}

void DatabaseSettingsPanel::setTotalSongs(int count) {
  juce::String text = juce::String(u8"จำนวนเพลงทั้งหมด : ") +
                      juce::String(count) + juce::String(u8" เพลง");
  totalSongsLabel.setText(text, juce::dontSendNotification);
}

//==============================================================================
VstiSettingsPanel::VstiSettingsPanel() {
  for (int i = 0; i < 8; ++i) {
    nameLabels[i].setText("Slot " + juce::String(i + 1) + ": Not Loaded",
                          juce::dontSendNotification);
    nameLabels[i].setColour(juce::Label::textColourId,
                            juce::Colour(220, 220, 220));
    addAndMakeVisible(nameLabels[i]);

    loadButtons[i].setButtonText("Load VSTi...");
    addAndMakeVisible(loadButtons[i]);
    loadButtons[i].onClick = [this, i]() {
      if (onLoadVstiClicked)
        onLoadVstiClicked(i + 1);
    };

    removeButtons[i].setButtonText("Remove");
    addAndMakeVisible(removeButtons[i]);
    removeButtons[i].onClick = [this, i]() {
      if (onRemoveVstiClicked)
        onRemoveVstiClicked(i + 1);
    };
  }
}

void VstiSettingsPanel::paint(juce::Graphics &g) {
  g.fillAll(juce::Colours::transparentBlack);
  g.setColour(juce::Colours::white);
  g.setFont(18.0f);
  g.drawText(u8"จัดการ VSTi (Virtual Instruments)", 10, 10, 300, 30,
             juce::Justification::centredLeft, true);
}

void VstiSettingsPanel::resized() {
  auto area = getLocalBounds().reduced(20);
  area.removeFromTop(40); // header space

  for (int i = 0; i < 8; ++i) {
    auto row = area.removeFromTop(30);
    nameLabels[i].setBounds(row.removeFromLeft(200));
    row.removeFromLeft(10);
    loadButtons[i].setBounds(row.removeFromLeft(100).reduced(0, 2));
    row.removeFromLeft(5);
    removeButtons[i].setBounds(row.removeFromLeft(80).reduced(0, 2));
    area.removeFromTop(5);
  }
}

//==============================================================================
SoundFontSettingsPanel::SoundFontSettingsPanel() {
  addAndMakeVisible(folderLabel);
  addAndMakeVisible(folderPathEditor);
  addAndMakeVisible(browseButton);

  folderLabel.setText(u8"โฟลเดอร์ SoundFont :", juce::dontSendNotification);
  folderPathEditor.setReadOnly(true);

  browseButton.onClick = [this]() {
    if (onBrowseClicked)
      onBrowseClicked();
  };
}

void SoundFontSettingsPanel::paint(juce::Graphics &g) {
  g.fillAll(juce::Colours::transparentBlack);
  g.setColour(juce::Colours::white);
  g.setFont(18.0f);
  g.drawText(u8"จัดการ SoundFont", 10, 10, 300, 30,
             juce::Justification::centredLeft, true);
}

void SoundFontSettingsPanel::resized() {
  auto area = getLocalBounds().reduced(20);
  area.removeFromTop(40);

  auto row = area.removeFromTop(30);
  folderLabel.setBounds(row.removeFromLeft(150));
  browseButton.setBounds(row.removeFromRight(40));
  row.removeFromRight(5);
  folderPathEditor.setBounds(row);
}

//==============================================================================
SettingsComponent::SettingsComponent() {
  addAndMakeVisible(tabs);

  tabs.addTab(u8"ทั่วไป", juce::Colours::transparentBlack, &generalPanel, false);
  tabs.addTab(u8"ฐานข้อมูลเพลง", juce::Colours::transparentBlack, &dbPanel,
              false);
  tabs.addTab(u8"อุปกรณ์เสียง", juce::Colours::transparentBlack, &soundDevicePanel,
              false);
  tabs.addTab(u8"ซาวด์ฟอนท์", juce::Colours::transparentBlack, &sf2Panel, false);

  sf2Button.setButtonText(u8"เลือก SoundFont");
  sf2Panel.addAndMakeVisible(sf2Button);
  sf2Button.setVisible(
      false); // Hide the old button as we move to folder-based system

  generalPanel.addAndMakeVisible(loadBgButton);
  loadBgButton.setButtonText(u8"เลือกภาพพื้นหลัง (Background)");

  dbPanel.ncnBrowseButton.onClick = [this]() {
    if (onLoadNcnClicked)
      onLoadNcnClicked();
  };

  dbPanel.pkmBrowseButton.onClick = [this]() {
    if (onLoadPkmClicked)
      onLoadPkmClicked();
  };

  dbPanel.buildDbButton.onClick = [this]() {
    if (onBuildDbClicked)
      onBuildDbClicked();
  };

  sf2Button.onClick = [this]() {
    if (onLoadSf2Clicked)
      onLoadSf2Clicked();
  };

  loadBgButton.onClick = [this]() {
    if (onLoadBgClicked)
      onLoadBgClicked();
  };
}

SettingsComponent::~SettingsComponent() {}

void SettingsComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff282B30));

  g.setColour(juce::Colours::white);
  g.setFont(18.0f);
  g.drawText("Settings", getLocalBounds().removeFromTop(30),
             juce::Justification::centred, true);
}

void SettingsComponent::resized() {
  auto area = getLocalBounds();
  area.removeFromTop(30);
  tabs.setBounds(area);

  sf2Button.setBounds(sf2Panel.getLocalBounds().withSizeKeepingCentre(200, 40));
  loadBgButton.setBounds(
      generalPanel.getLocalBounds().withSizeKeepingCentre(200, 40));
}

void SettingsComponent::setOnLoadNcnClicked(std::function<void()> callback) {
  onLoadNcnClicked = callback;
}

void SettingsComponent::setOnLoadPkmClicked(std::function<void()> callback) {
  onLoadPkmClicked = callback;
}

void SettingsComponent::setOnBuildDbClicked(std::function<void()> callback) {
  onBuildDbClicked = callback;
}

void SettingsComponent::setOnLoadSf2Clicked(std::function<void()> callback) {
  onLoadSf2Clicked = callback;
}

void SettingsComponent::setOnLoadBgClicked(std::function<void()> callback) {
  onLoadBgClicked = callback;
}

void SettingsComponent::setNcnPath(const juce::String &path) {
  dbPanel.ncnPathEditor.setText(path);
}

void SettingsComponent::setPkmPath(const juce::String &path) {
  dbPanel.pkmPathEditor.setText(path);
}

void SettingsComponent::setDbProgress(double progress) {
  dbPanel.updateProgress(progress);
}

void SettingsComponent::setTotalSongs(int count) {
  dbPanel.setTotalSongs(count);
}

void SettingsComponent::setSf2ButtonText(const juce::String &text) {
  sf2Button.setButtonText(text);
}
