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

SettingsComponent::SettingsComponent() {
  addAndMakeVisible(tabs);

  tabs.addTab(u8"ทั่วไป", juce::Colours::transparentBlack, &generalPanel, false);
  tabs.addTab(u8"ฐานข้อมูลเพลง", juce::Colours::transparentBlack, &dbPanel,
              false);
  tabs.addTab(u8"อุปกรณ์เสียง", juce::Colours::transparentBlack, &soundDevicePanel,
              false);
  tabs.addTab(u8"ซาวด์ฟอนท์", juce::Colours::transparentBlack, &sf2Panel, false);

  sf2Panel.addAndMakeVisible(sf2Button);
  sf2Button.setButtonText(u8"เลือก SoundFont");

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
