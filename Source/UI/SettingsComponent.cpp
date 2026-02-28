#include "UI/SettingsComponent.h"

SettingsComponent::SettingsComponent() {
  addAndMakeVisible(loadDbButton);
  addAndMakeVisible(sf2Button);

  loadDbButton.onClick = [this]() {
    if (onLoadDbClicked)
      onLoadDbClicked();
  };

  sf2Button.onClick = [this]() {
    if (onLoadSf2Clicked)
      onLoadSf2Clicked();
  };
}

SettingsComponent::~SettingsComponent() {}

void SettingsComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colours::darkgrey.withAlpha(0.9f));
  g.setColour(juce::Colours::white);
  g.setFont(20.0f);
  g.drawText("Settings", getLocalBounds().removeFromTop(40),
             juce::Justification::centred, true);
}

void SettingsComponent::resized() {
  auto area = getLocalBounds().reduced(20);
  area.removeFromTop(40); // header

  loadDbButton.setBounds(area.removeFromTop(40));
  area.removeFromTop(10);
  sf2Button.setBounds(area.removeFromTop(40));
}

void SettingsComponent::setOnLoadDbClicked(std::function<void()> callback) {
  onLoadDbClicked = callback;
}

void SettingsComponent::setOnLoadSf2Clicked(std::function<void()> callback) {
  onLoadSf2Clicked = callback;
}

void SettingsComponent::setDbButtonText(const juce::String &text) {
  loadDbButton.setButtonText(text);
}

void SettingsComponent::setSf2ButtonText(const juce::String &text) {
  sf2Button.setButtonText(text);
}
