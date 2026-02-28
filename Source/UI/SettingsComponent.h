#pragma once

#include <JuceHeader.h>
#include <functional>

class SettingsComponent : public juce::Component {
public:
  SettingsComponent();
  ~SettingsComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  // Set callbacks
  void setOnLoadDbClicked(std::function<void()> callback);
  void setOnLoadSf2Clicked(std::function<void()> callback);

  // Update button texts
  void setDbButtonText(const juce::String &text);
  void setSf2ButtonText(const juce::String &text);

private:
  juce::TextButton loadDbButton{"Load NCN Folder"};
  juce::TextButton sf2Button{"Select SoundFont"};

  std::function<void()> onLoadDbClicked;
  std::function<void()> onLoadSf2Clicked;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsComponent)
};
