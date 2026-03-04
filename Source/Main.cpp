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
    // Setup File Logger - must be member to outlive initialise() scope
    auto logFile =
        juce::File::getSpecialLocation(juce::File::currentExecutableFile)
            .getSiblingFile("debug_log.txt");
    fileLogger.reset(new juce::FileLogger(logFile, "PK Karaoke Debug Log", 0));
    juce::Logger::setCurrentLogger(fileLogger.get());

    juce::Logger::writeToLog("--- Application Starting ---");

    // Force Embedded Thai Font globally for JUCE 8
    juce::Typeface::Ptr tahomaTypeface =
        juce::Typeface::createSystemTypefaceFor(BinaryData::tahoma_ttf,
                                                BinaryData::tahoma_ttfSize);

    if (tahomaTypeface != nullptr) {
      juce::LookAndFeel::getDefaultLookAndFeel().setDefaultSansSerifTypeface(
          tahomaTypeface);
    }

    juce::Logger::writeToLog("Creating Splash Screen...");

    // Show splash FIRST, before audio init (avoid audio dialog hiding splash)
    splashComp.reset(new SplashComponent(karaokeEngine, [this]() {
      juce::Logger::writeToLog("Splash finished -> Creating Main Window");
      juce::MessageManager::callAsync([this]() {
        splashComp->removeFromDesktop();
        splashComp = nullptr;
        mainWindow.reset(new MainWindow(getApplicationName(), &karaokeEngine));
      });
    }));

    // Add directly to desktop - use drop shadow only (no title bar)
    splashComp->addToDesktop(juce::ComponentPeer::windowHasDropShadow);

    // Centre on primary display
    if (auto *display =
            juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()) {
      auto area = display->userArea;
      splashComp->setBounds(area.getCentreX() - 300, area.getCentreY() - 200,
                            600, 400);
    }
    splashComp->setVisible(true);
    splashComp->toFront(false);
    splashComp->repaint();

    juce::Logger::writeToLog("Splash visible, starting audio init...");

    // Set up Audio AFTER splash is visible
    audioDeviceManager.initialiseWithDefaultDevices(2, 2);
    audioPlayer.setProcessor(&karaokeEngine);
    audioDeviceManager.addAudioCallback(&audioPlayer);

    juce::Logger::writeToLog("Audio initialized, splash running.");
  }

  void shutdown() override {
    juce::Logger::writeToLog("--- Shutdown ---");
    audioDeviceManager.removeAudioCallback(&audioPlayer);
    audioPlayer.setProcessor(nullptr);
    if (splashComp) {
      splashComp->removeFromDesktop();
      splashComp = nullptr;
    }
    mainWindow = nullptr;
    juce::Logger::setCurrentLogger(
        nullptr); // Must null BEFORE fileLogger destroyed
    fileLogger = nullptr;
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

      // Set window icon
      int logoSize = 0;
      if (const char *logoData =
              BinaryData::getNamedResource("Logo_png", logoSize)) {
        setIcon(juce::ImageFileFormat::loadFrom(logoData, (size_t)logoSize));
      }

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
  std::unique_ptr<juce::FileLogger> fileLogger;
  std::unique_ptr<SplashComponent> splashComp;
  std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(PKKaraokeApplication)
