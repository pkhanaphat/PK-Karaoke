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
class VstiSettingsPanel : public juce::Component {
public:
  VstiSettingsPanel();
  ~VstiSettingsPanel() override = default;

  void paint(juce::Graphics &g) override;
  void resized() override;

  // อัปเดตรายการ Plugin ที่แสกนพบ
  void
  setAvailablePlugins(const juce::OwnedArray<juce::PluginDescription> &plugins);

  // Enable/Disable ปุ่มตามสถานะการโหลด Plugin (slotIndex 1-8)
  void updateSlotState(int slotIndex, bool pluginLoaded);

  juce::TextButton loadButtons[8];
  juce::TextButton openButtons[8]; // เปิดหน้าต่าง UI ของ VSTi
  juce::TextButton removeButtons[8];
  juce::Label nameLabels[8];

  std::function<void(int, const juce::PluginDescription &)> onPluginSelected;
  std::function<void(int)> onRemoveVstiClicked;
  std::function<void(int)> onOpenVstiClicked; // เปิดหน้าต่าง VSTi UI

private:
  juce::Array<juce::PluginDescription> availablePlugins;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VstiSettingsPanel)
};

//==============================================================================
class SoundFontSettingsPanel : public juce::Component {
public:
  SoundFontSettingsPanel();
  ~SoundFontSettingsPanel() override = default;

  void paint(juce::Graphics &g) override;
  void resized() override;

  // SF2 Folder path editor
  juce::TextEditor folderPathEditor;
  juce::TextButton browseButton{"..."};

  // Global SF2 ComboBox (เหมือนใน SynthMixer)
  juce::ComboBox globalSf2ComboBox;
  void refreshGlobalSf2List(const juce::StringArray &sf2Names);
  void setSelectedGlobalSf2(const juce::String &sf2Name);

  std::function<void()> onBrowseClicked;
  std::function<void(const juce::String &)> onGlobalSf2Changed; // จาก ComboBox

private:
  juce::Label folderLabel;
  juce::Label globalSf2Label;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundFontSettingsPanel)
};

#include "UI/VstScannerComponent.h"

//==============================================================================
class SettingsComponent : public juce::Component {
public:
  SettingsComponent(PluginHost &host);
  ~SettingsComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  // Set callbacks
  void setOnLoadNcnClicked(std::function<void()> callback);
  void setOnLoadPkmClicked(std::function<void()> callback);
  void setOnBuildDbClicked(std::function<void()> callback);
  void setOnLoadBgClicked(std::function<void()> callback);

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

  SoundFontSettingsPanel sf2Panel;

private:
  juce::TabbedComponent tabs{juce::TabbedButtonBar::TabsAtTop};

  DatabaseSettingsPanel dbPanel;
  VstScannerComponent vstScannerPanel;

  // Future panels
  juce::Component generalPanel;
  juce::Component soundDevicePanel;
  juce::TextButton
      sf2Button; // Keep for backward compat if needed, or remove later

  std::function<void()> onLoadNcnClicked;
  std::function<void()> onLoadPkmClicked;
  std::function<void()> onBuildDbClicked;
  std::function<void()> onLoadSf2Clicked;
  std::function<void()> onLoadBgClicked;

  juce::TextButton loadBgButton{"Load Background Image"};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsComponent)
};
