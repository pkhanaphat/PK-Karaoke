#pragma once

#include "Core/KaraokeEngine.h"
#include "Database/DatabaseManager.h"
#include "Database/LibraryScanner.h"
#include "Services/YouTubeService.h"
#include "UI/BottomBarComponent.h"
#include "UI/LookAndFeel_PK.h"
#include "UI/LyricsView.h"
#include "UI/QueueComponent.h"
#include "UI/SettingsComponent.h"
#include "UI/SongListView.h"
#include "UI/SynthMixerWindow.h"
#include "UI/YouTubePlayerComponent.h"
#include <JuceHeader.h>

class MainComponent : public juce::Component, private juce::Timer {
public:
  MainComponent(KaraokeEngine &engine);
  ~MainComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;
  void timerCallback() override;
  void mouseDown(const juce::MouseEvent &event) override;

private:
  KaraokeEngine &karaokeEngine;
  YouTubeService youtubeService;

  // UI Components
  LyricsView lyricsView;
  SongListView songListView;
  YouTubePlayerComponent youtubePlayer;
  QueueComponent queueComponent{karaokeEngine.getQueueManager()};

  std::unique_ptr<BottomBarComponent> bottomBar;

  std::unique_ptr<SynthMixerWindow> mixerWindow;
  std::unique_ptr<juce::DocumentWindow> settingsWindow;
  SettingsComponent settingsComponent;
  std::unique_ptr<juce::DocumentWindow> vstiSettingsWindow;
  VstiSettingsPanel vstiSettingsPanel;
  std::unique_ptr<juce::FileChooser> chooser;

  LookAndFeel_PK_Modern lookAndFeel;

  DatabaseManager dbManager;
  LibraryScanner libraryScanner{dbManager};

  // Hover panel state
  bool isSongListVisible = false;
  bool isQueueVisible = false;
  bool isBottomBarVisible = false;

  int leftPanelHideTimerMs = 0;
  int rightPanelHideTimerMs = 0;
  int bottomPanelHideTimerMs = 0;

  juce::Rectangle<int> songListBounds;
  juce::Rectangle<int> queueBounds;
  juce::Rectangle<int> bottomBarBounds;

  void showNativePopup(juce::Component *comp, juce::Rectangle<int> bounds,
                       bool isTemporary = true);
  void hideNativePopup(juce::Component *comp);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
