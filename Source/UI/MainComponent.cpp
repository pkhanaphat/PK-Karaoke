#include "UI/MainComponent.h"

// ============================================================
// Helpers: บันทึก/โหลด settings ด้วย JUCE PropertiesFile
// ============================================================
static juce::PropertiesFile::Options getSettingsOptions() {
  juce::PropertiesFile::Options opts;
  opts.applicationName = "PKKaraokePlayer";
  opts.filenameSuffix = ".settings";
  opts.osxLibrarySubFolder = "Application Support";
  return opts;
}

static juce::ApplicationProperties &appProps() {
  static juce::ApplicationProperties props;
  static bool initialised = false;
  if (!initialised) {
    props.setStorageParameters(getSettingsOptions());
    initialised = true;
  }
  return props;
}

static void saveSetting(const juce::String &key, const juce::String &value) {
  if (auto *p = appProps().getUserSettings())
    p->setValue(key, value);
  appProps().saveIfNeeded();
}

static juce::String loadSetting(const juce::String &key) {
  if (auto *p = appProps().getUserSettings())
    return p->getValue(key);
  return {};
}

//==============================================================================
MainComponent::MainComponent(KaraokeEngine &engine) : karaokeEngine(engine) {
  juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);
  setLookAndFeel(&lookAndFeel);

  songListView.setLookAndFeel(&lookAndFeel);
  queueComponent.setLookAndFeel(&lookAndFeel);
  // bottomBar is a unique_ptr, will set after creating it.

  setSize(1000, 700);

  // UI Initialization
  addAndMakeVisible(youtubePlayer);
  youtubePlayer.setVisible(
      false); // Hide by default so it doesn't show a white screen
  addAndMakeVisible(lyricsView);
  addAndMakeVisible(songListView);
  addAndMakeVisible(queueComponent);

  // Set initial visibility of side panels to false
  songListView.setVisible(false);
  queueComponent.setVisible(false);

  // Settings Panel Initialization
  settingsComponent.setOnLoadDbClicked([this]() {
    juce::String lastFolder = loadSetting("lastMusicFolder");
    juce::File startDir =
        lastFolder.isNotEmpty() && juce::File(lastFolder).isDirectory()
            ? juce::File(lastFolder)
            : juce::File::getSpecialLocation(juce::File::userHomeDirectory);

    chooser = std::make_unique<juce::FileChooser>("Select NCN Karaoke Folder",
                                                  startDir);

    auto chooserFlags = juce::FileBrowserComponent::canSelectDirectories;

    chooser->launchAsync(chooserFlags, [this](const juce::FileChooser &fc) {
      auto dir = fc.getResult();
      if (dir.isDirectory()) {
        saveSetting("lastMusicFolder", dir.getFullPathName());
        juce::MessageManager::callAsync(
            [this, dir]() { libraryScanner.startScan(dir); });
      }
    });
  });

  settingsComponent.setOnLoadSf2Clicked([this]() {
    juce::String lastSF2 = loadSetting("lastSF2Path");
    juce::File startDir =
        lastSF2.isNotEmpty()
            ? juce::File(lastSF2).getParentDirectory()
            : juce::File::getSpecialLocation(juce::File::userHomeDirectory);

    chooser = std::make_unique<juce::FileChooser>("Select SoundFont (.sf2)",
                                                  startDir, "*.sf2");

    auto chooserFlags = juce::FileBrowserComponent::openMode |
                        juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync(chooserFlags, [this](const juce::FileChooser &fc) {
      auto file = fc.getResult();
      if (file.existsAsFile()) {
        saveSetting("lastSF2Path", file.getFullPathName());
        juce::MessageManager::callAsync([this, file]() {
          karaokeEngine.loadSoundFont(file);
          settingsComponent.setSf2ButtonText(file.getFileName());
        });
      }
    });
  });

  // Start the timer to track mouse hover (100ms interval)
  startTimer(100);

  // Setup Bottom Bar
  bottomBar = std::make_unique<BottomBarComponent>();
  bottomBar->setLookAndFeel(&lookAndFeel);

  bottomBar->onPlayClicked = [this]() { karaokeEngine.play(); };
  bottomBar->onStopClicked = [this]() { karaokeEngine.stop(); };
  bottomBar->onNextClicked = [this]() {};

  bottomBar->onSettingsClicked = [this]() {
    if (settingsWindow == nullptr) {
      settingsWindow = std::make_unique<juce::DocumentWindow>(
          "Settings", juce::Colours::black, juce::DocumentWindow::closeButton);
      settingsWindow->setContentNonOwned(&settingsComponent, true);
      settingsWindow->centreWithSize(400, 300);
      settingsWindow->setUsingNativeTitleBar(true);
    }
    settingsWindow->setVisible(!settingsWindow->isVisible());
  };

  bottomBar->onMixerClicked = [this]() {
    if (mixerWindow == nullptr)
      mixerWindow = std::make_unique<SynthMixerWindow>(
          "Synth Mixer", karaokeEngine.getMixerController());

    mixerWindow->setVisible(!mixerWindow->isVisible());
  };

  lyricsView.attachToPlayer(&karaokeEngine.getMidiPlayer());

  karaokeEngine.onSongLoaded = [this]() {
    juce::MessageManager::callAsync(
        [this]() { lyricsView.setLyrics(karaokeEngine.getCurrentLyrics()); });
  };

  // Initialize Database
  juce::File dbFile =
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("PKKaraoke")
          .getChildFile("songs.db");

  if (dbManager.initialize(dbFile)) {
    songListView.setDatabaseManager(&dbManager);
    DBG("Database OK, count: " + juce::String(dbManager.getTotalCount()));
  }

  // Set up Scanner Callbacks
  libraryScanner.onStatusChanged = [this](juce::String status) {
    juce::MessageManager::callAsync(
        [this, status]() { settingsComponent.setDbButtonText(status); });
  };

  libraryScanner.onScanFinished = [this]() {
    juce::MessageManager::callAsync([this]() {
      settingsComponent.setDbButtonText(
          "DB Created (" + juce::String(dbManager.getTotalCount()) + " songs)");
      songListView.refreshList();
    });
  };

  // ─── โหลด paths ที่บันทึกไว้ก่อนหน้า ───────────────────────────────────────
  {
    juce::String lastSF2 = loadSetting("lastSF2Path");
    if (lastSF2.isNotEmpty()) {
      juce::File sf2File(lastSF2);
      if (sf2File.existsAsFile()) {
        karaokeEngine.loadSoundFont(sf2File);
        settingsComponent.setSf2ButtonText(sf2File.getFileName());
        DBG("Auto-loaded SF2: " + lastSF2);
      }
    }

    juce::String lastMusic = loadSetting("lastMusicFolder");
    if (lastMusic.isNotEmpty()) {
      juce::File musicDir(lastMusic);
      if (musicDir.isDirectory()) {
        settingsComponent.setDbButtonText(musicDir.getFileName() + " (saved)");
        DBG("Last music folder: " + lastMusic);
      }
    }
  }

  // Button callbacks (Now handled in BottomBarComponent)

  // List callback
  songListView.onSongSelected = [this](const SongRecord &entry) {
    bool wasEmpty = karaokeEngine.getQueueManager().isEmpty();
    karaokeEngine.getQueueManager().addSong(entry);

    if (wasEmpty && !karaokeEngine.getMidiPlayer().isPlaying()) {
      karaokeEngine.play();
    }
  };
}

MainComponent::~MainComponent() {
  stopTimer();
  appProps().saveIfNeeded(); // บันทึกก่อนปิด
}

//==============================================================================
void MainComponent::timerCallback() {
  // Get logical mouse position relative to this component.
  // This automatically accounts for UI scaling/zoom.
  auto mousePos = getMouseXYRelative();

  // Logical threshold for triggering hover at edges
  const int edgeThreshold = 15;
  const int headerHeight = 40;

  // Only trigger if mouse is actually inside the window logically.
  // We add a 5 pixel tolerance because getLocalBounds().contains() fails if the
  // mouse is pegged exactly at the maximum right or bottom edge in true
  // fullscreen.
  bool isInsideWindow = (mousePos.x >= -5 && mousePos.x <= getWidth() + 5 &&
                         mousePos.y >= -5 && mousePos.y <= getHeight() + 5);

  // --- Left Panel (Song List) Logic ---
  if (!isSongListVisible && mousePos.x < edgeThreshold && mousePos.y > 0 &&
      isInsideWindow) {
    isSongListVisible = true;
    leftPanelHideTimerMs = 0;
    showNativePopup(&songListView, songListBounds);
  } else if (isSongListVisible) {
    auto screenMousePos = juce::Desktop::getInstance().getMousePosition();
    if (songListBounds.translated(getScreenX(), getScreenY())
            .expanded(20, 20)
            .contains(screenMousePos)) {
      leftPanelHideTimerMs = 0;
    } else {
      leftPanelHideTimerMs += 100;
      if (leftPanelHideTimerMs >= 2000) {
        isSongListVisible = false;
        hideNativePopup(&songListView);
      }
    }
  }

  // --- Right Panel (Queue) Logic ---
  if (!isQueueVisible && mousePos.x > getWidth() - edgeThreshold &&
      mousePos.y > 0 && isInsideWindow) {
    isQueueVisible = true;
    rightPanelHideTimerMs = 0;
    showNativePopup(&queueComponent, queueBounds);
  } else if (isQueueVisible) {
    auto screenMousePos = juce::Desktop::getInstance().getMousePosition();
    if (queueBounds.translated(getScreenX(), getScreenY())
            .expanded(20, 20)
            .contains(screenMousePos)) {
      rightPanelHideTimerMs = 0;
    } else {
      rightPanelHideTimerMs += 100;
      if (rightPanelHideTimerMs >= 2000) {
        isQueueVisible = false;
        hideNativePopup(&queueComponent);
      }
    }
  }

  // --- Bottom Panel (Controls) Logic ---
  bottomBarBounds = juce::Rectangle<int>(0, getHeight() - 70, getWidth(), 70);
  if (!isBottomBarVisible && mousePos.y > getHeight() - edgeThreshold &&
      isInsideWindow) {
    isBottomBarVisible = true;
    bottomPanelHideTimerMs = 0;
    showNativePopup(bottomBar.get(), bottomBarBounds);
  } else if (isBottomBarVisible) {
    auto screenMousePos = juce::Desktop::getInstance().getMousePosition();
    if (bottomBarBounds.translated(getScreenX(), getScreenY())
            .expanded(20, 20)
            .contains(screenMousePos)) {
      bottomPanelHideTimerMs = 0;
    } else {
      bottomPanelHideTimerMs += 100;
      if (bottomPanelHideTimerMs >= 2000) {
        isBottomBarVisible = false;
        hideNativePopup(bottomBar.get());
      }
    }
  }
}

//==============================================================================
void MainComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colours::black);
}

void MainComponent::resized() {
  auto area = getLocalBounds();

  int panelWidth = 350;
  songListBounds = juce::Rectangle<int>(0, 0, panelWidth, getHeight());
  queueBounds =
      juce::Rectangle<int>(getWidth() - panelWidth, 0, panelWidth, getHeight());
  bottomBarBounds = juce::Rectangle<int>(0, getHeight() - 70, getWidth(), 70);

  // For WebView2, it must be exactly the size of the container, no shrinking
  youtubePlayer.setBounds(area);
  lyricsView.setBounds(area);
  lyricsView.toFront(false);

  // If popups are active during a resize (e.g., window restored), update their
  // bounds natively
  if (isSongListVisible) {
    showNativePopup(&songListView, songListBounds);
  }
  if (isQueueVisible) {
    showNativePopup(&queueComponent, queueBounds);
  }
  if (isBottomBarVisible) {
    showNativePopup(bottomBar.get(), bottomBarBounds);
  }
}

void MainComponent::showNativePopup(juce::Component *comp,
                                    juce::Rectangle<int> bounds) {
  if (comp == nullptr)
    return;

  // Position it in global screen coordinates
  juce::Rectangle<int> screenBounds =
      bounds.translated(getScreenX(), getScreenY());

  if (comp->isOnDesktop()) {
    comp->setBounds(screenBounds);
  } else {
    comp->addToDesktop(juce::ComponentPeer::windowIsTemporary |
                       juce::ComponentPeer::windowIgnoresKeyPresses |
                       juce::ComponentPeer::windowHasDropShadow);
    comp->setBounds(screenBounds);
    comp->setOpaque(false); // Enable transparency
    comp->setVisible(true);
    comp->toFront(true);
  }
}

void MainComponent::hideNativePopup(juce::Component *comp) {
  if (comp != nullptr && comp->isOnDesktop()) {
    comp->removeFromDesktop();
    comp->setVisible(false);
  }
}
