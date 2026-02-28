#include "Core/KaraokeEngine.h"
#include "UI/MainComponent.h"
#include <JuceHeader.h>

class PKKaraokeApplication : public juce::JUCEApplication {
public:
  PKKaraokeApplication() {}

  const juce::String getApplicationName() override {
    return ProjectInfo::projectName;
  }
  const juce::String getApplicationVersion() override {
    return ProjectInfo::versionString;
  }
  bool moreThanOneInstanceAllowed() override { return true; }

  void initialise(const juce::String &commandLine) override {
    // Set up Audio
    audioDeviceManager.initialiseWithDefaultDevices(2, 2);
    audioPlayer.setProcessor(&karaokeEngine);
    audioDeviceManager.addAudioCallback(&audioPlayer);

    mainWindow.reset(new MainWindow(getApplicationName(), &karaokeEngine));
  }

  void shutdown() override {
    audioDeviceManager.removeAudioCallback(&audioPlayer);
    audioPlayer.setProcessor(nullptr);
    mainWindow = nullptr;
  }

  void systemRequestedQuit() override { quit(); }

  void anotherInstanceStarted(const juce::String &commandLine) override {}

  class MainWindow : public juce::DocumentWindow {
  public:
    MainWindow(juce::String name, KaraokeEngine *engine)
        : DocumentWindow(
              name,
              juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(
                  juce::ResizableWindow::backgroundColourId),
              DocumentWindow::allButtons) {
      setUsingNativeTitleBar(false);
      setContentOwned(new MainComponent(*engine), true);

      setResizable(true, true);

      // Use a true borderless fullscreen window instead of Kiosk Mode
      // Kiosk Mode aggressively minimises when it loses focus (e.g. Windows key
      // pressed)
      setTitleBarHeight(0);
      setFullScreen(true);

      setVisible(true);
    }

    void closeButtonPressed() override {
      juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
  };

private:
  KaraokeEngine karaokeEngine;
  juce::AudioDeviceManager audioDeviceManager;
  juce::AudioProcessorPlayer audioPlayer;
  std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(PKKaraokeApplication)
