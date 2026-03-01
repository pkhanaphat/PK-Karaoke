#pragma once

#include <JuceHeader.h>
#include <functional>

//==============================================================================
class DatabaseSettingsPanel : public juce::Component {
public:
  DatabaseSettingsPanel();
  ~DatabaseSettingsPanel() override = default;

  void paint(juce::Graphics &g) override;
  void resized() override;

  juce::TextEditor ncnPathEditor;
  juce::TextButton ncnBrowseButton{"..."};

  juce::TextEditor pkmPathEditor;
  juce::TextButton pkmBrowseButton{"..."};

  juce::ProgressBar progressBar;
  double progress = 0.0;

  juce::TextButton buildDbButton;
  juce::Label totalSongsLabel;

  void updateProgress(double newProgress);
  void setTotalSongs(int count);

private:
  juce::Label ncnLabel;
  juce::Label pkmLabel;
  juce::Label progressLabel;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DatabaseSettingsPanel)
};

//==============================================================================
class SettingsComponent : public juce::Component {
public:
  SettingsComponent();
  ~SettingsComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  // Set callbacks
  void setOnLoadNcnClicked(std::function<void()> callback);
  void setOnLoadPkmClicked(std::function<void()> callback);
  void setOnBuildDbClicked(std::function<void()> callback);

  // Backwards compatibility / other tabs
  void setOnLoadSf2Clicked(std::function<void()> callback);

  // Update UI
  void setNcnPath(const juce::String &path);
  void setPkmPath(const juce::String &path);
  void setDbProgress(double progress);
  void setTotalSongs(int count);
  void setSf2ButtonText(const juce::String &text);

  // Temporary alias for old methods to avoid breaking MainComponent immediately
  void setDbButtonText(const juce::String &text) {};
  void setOnLoadDbClicked(std::function<void()> callback) {
    setOnLoadNcnClicked(callback);
  }

private:
  juce::TabbedComponent tabs{juce::TabbedButtonBar::TabsAtTop};

  DatabaseSettingsPanel dbPanel;

  // Future panels
  juce::Component generalPanel;
  juce::Component soundDevicePanel;
  juce::Component sf2Panel;
  juce::TextButton sf2Button;

  std::function<void()> onLoadNcnClicked;
  std::function<void()> onLoadPkmClicked;
  std::function<void()> onBuildDbClicked;
  std::function<void()> onLoadSf2Clicked;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsComponent)
};
