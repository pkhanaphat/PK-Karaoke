#include "BinaryData.h"
#include "Core/KaraokeEngine.h"
#include "UI/MainComponent.h"
#include "UI/SplashComponent.h"
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
    // Force Embedded Thai Font globally for JUCE 8
    juce::Typeface::Ptr tahomaTypeface =
        juce::Typeface::createSystemTypefaceFor(BinaryData::tahoma_ttf,
                                                BinaryData::tahoma_ttfSize);

    if (tahomaTypeface != nullptr) {
      juce::LookAndFeel::getDefaultLookAndFeel().setDefaultSansSerifTypeface(
          tahomaTypeface);
    }

    // Set up Audio
    audioDeviceManager.initialiseWithDefaultDevices(2, 2);
    audioPlayer.setProcessor(&karaokeEngine);
    audioDeviceManager.addAudioCallback(&audioPlayer);

    splashWindow.reset(new SplashWindow(
        "Starting PK Karaoke Player...", &karaokeEngine, [this]() {
          juce::MessageManager::callAsync([this]() {
            splashWindow = nullptr;
            mainWindow.reset(
                new MainWindow(getApplicationName(), &karaokeEngine));
          });
        }));
  }

  void shutdown() override {
    audioDeviceManager.removeAudioCallback(&audioPlayer);
    audioPlayer.setProcessor(nullptr);
    splashWindow = nullptr;
    mainWindow = nullptr;
  }

  void systemRequestedQuit() override { quit(); }

  void anotherInstanceStarted(const juce::String &commandLine) override {}

  class SplashWindow : public juce::DocumentWindow {
  public:
    SplashWindow(juce::String name, KaraokeEngine *engine,
                 std::function<void()> onFinished)
        : DocumentWindow(name, juce::Colours::black, 0) {
      setUsingNativeTitleBar(false);
      setTitleBarHeight(0);
      setDropShadowEnabled(true);
      setAlwaysOnTop(true);

      setContentOwned(new SplashComponent(*engine, onFinished), true);
      centreWithSize(getWidth(), getHeight());
      setVisible(true);
    }
    void closeButtonPressed() override {}
  };

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

      // Disable resizing to prevent double-click from restoring down/shrinking
      // the fullscreen window.
      setResizable(false, false);

      // Use a true borderless fullscreen window instead of Kiosk Mode
      // Kiosk Mode aggressively minimises when it loses focus (e.g. Windows key
      // pressed)
      setTitleBarHeight(0);

      // Get the display where the mouse is currently located and fill its total
      // area
      auto &desktop = juce::Desktop::getInstance();
      auto display =
          desktop.getDisplays().findDisplayForPoint(desktop.getMousePosition());
      setBounds(display.totalArea);

      // Prevent the window from being dragged by clicking its content since we
      // have no title bar
      setDraggable(false);

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
  std::unique_ptr<SplashWindow> splashWindow;
  std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(PKKaraokeApplication)
