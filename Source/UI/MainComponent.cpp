#include "UI/MainComponent.h"
#include "UI/SettingsComponent.h"

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
class SettingsWindowContainer : public juce::DocumentWindow {
public:
  SettingsWindowContainer(const juce::String &name,
                          juce::Colour backgroundColour, int requiredButtons)
      : DocumentWindow(name, backgroundColour, requiredButtons) {}

  void closeButtonPressed() override { setVisible(false); }
};

//==============================================================================
MainComponent::MainComponent(KaraokeEngine &engine) : karaokeEngine(engine) {
  juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);

  bottomBar = std::make_unique<BottomBarComponent>();
  juce::Logger::writeToLog("MainComponent: Start constructor");

  // Explicitly set look and feel for member components created before
  // setDefaultLookAndFeel
  settingsComponent.setLookAndFeel(&lookAndFeel);
  vstiSettingsPanel.setLookAndFeel(&lookAndFeel);
  lyricsView.setLookAndFeel(&lookAndFeel);
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
  settingsComponent.setOnLoadNcnClicked([this]() {
    juce::String lastFolder = loadSetting("lastNcnFolder");
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
        saveSetting("lastNcnFolder", dir.getFullPathName());
        settingsComponent.setNcnPath(dir.getFullPathName());
      }
    });
  });

  settingsComponent.setOnLoadPkmClicked([this]() {
    juce::String lastFolder = loadSetting("lastPkmFolder");
    juce::File startDir =
        lastFolder.isNotEmpty() && juce::File(lastFolder).isDirectory()
            ? juce::File(lastFolder)
            : juce::File::getSpecialLocation(juce::File::userHomeDirectory);

    chooser = std::make_unique<juce::FileChooser>("Select PKM Karaoke Folder",
                                                  startDir);

    auto chooserFlags = juce::FileBrowserComponent::canSelectDirectories;

    chooser->launchAsync(chooserFlags, [this](const juce::FileChooser &fc) {
      auto dir = fc.getResult();
      if (dir.isDirectory()) {
        saveSetting("lastPkmFolder", dir.getFullPathName());
        settingsComponent.setPkmPath(dir.getFullPathName());
      }
    });
  });

  settingsComponent.setOnBuildDbClicked([this]() {
    juce::String ncnPath = loadSetting("lastNcnFolder");
    if (ncnPath.isNotEmpty()) {
      juce::File ncnDir(ncnPath);
      if (ncnDir.isDirectory()) {
        juce::MessageManager::callAsync(
            [this, ncnDir]() { libraryScanner.startScan(ncnDir); });
      }
    }
  });

  settingsComponent.sf2Panel.onBrowseClicked = [this]() {
    juce::String lastSF2Dir = loadSetting("lastSF2Folder");
    juce::File startDir =
        lastSF2Dir.isNotEmpty() && juce::File(lastSF2Dir).isDirectory()
            ? juce::File(lastSF2Dir)
            : juce::File::getSpecialLocation(juce::File::userHomeDirectory);

    chooser = std::make_unique<juce::FileChooser>("Select SoundFont Folder",
                                                  startDir);

    auto chooserFlags = juce::FileBrowserComponent::canSelectDirectories;

    chooser->launchAsync(chooserFlags, [this](const juce::FileChooser &fc) {
      auto dir = fc.getResult();
      if (dir.isDirectory()) {
        saveSetting("lastSF2Folder", dir.getFullPathName());
        settingsComponent.sf2Panel.folderPathEditor.setText(
            dir.getFullPathName(), false);

        // Update MixerController
        karaokeEngine.getMixerController().setSF2Directory(dir);

        // Refresh Mixer UI if it exists
        if (mixerWindow != nullptr) {
          mixerWindow->getMixerComponent().updateAllStrips();
        }
      }
    });
  };

  // Set initial text for SF2 folder
  juce::String savedSf2Folder = loadSetting("lastSF2Folder");
  settingsComponent.sf2Panel.folderPathEditor.setText(savedSf2Folder, false);
  if (savedSf2Folder.isNotEmpty()) {
    karaokeEngine.getMixerController().setSF2Directory(
        juce::File(savedSf2Folder));
  }

  settingsComponent.setOnLoadBgClicked([this]() {
    juce::String lastBg = loadSetting("lastBgPath");
    juce::File startDir =
        lastBg.isNotEmpty()
            ? juce::File(lastBg).getParentDirectory()
            : juce::File::getSpecialLocation(juce::File::userPicturesDirectory);

    chooser = std::make_unique<juce::FileChooser>(
        "Select Background Image", startDir, "*.png;*.jpg;*.jpeg");

    auto chooserFlags = juce::FileBrowserComponent::openMode |
                        juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync(chooserFlags, [this](const juce::FileChooser &fc) {
      auto file = fc.getResult();
      if (file.existsAsFile()) {
        saveSetting("lastBgPath", file.getFullPathName());
        juce::Image newImage = juce::ImageCache::getFromFile(file);
        if (newImage.isValid()) {
          juce::MessageManager::callAsync([this, newImage]() {
            lyricsView.setCustomBackgroundImage(newImage);
          });
        }
      }
    });
  });

  vstiSettingsPanel.onLoadVstiClicked = [this](int slotIndex) {
    juce::String lastVst = loadSetting("lastVstPath");
    juce::File startDir =
        lastVst.isNotEmpty()
            ? juce::File(lastVst).getParentDirectory()
            : juce::File::getSpecialLocation(juce::File::userHomeDirectory);

    chooser = std::make_unique<juce::FileChooser>("Select VSTi3 Instrument",
                                                  startDir, "*.vst3");
    auto flags = juce::FileBrowserComponent::openMode |
                 juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync(flags, [this, slotIndex](const juce::FileChooser &fc) {
      auto file = fc.getResult();
      if (file.existsAsFile()) {
        saveSetting("lastVstPath", file.getFullPathName());
        juce::MessageManager::callAsync([this, slotIndex, file]() {
          if (karaokeEngine.getGraphManager().loadVstiPlugin(
                  slotIndex, file.getFullPathName())) {
            vstiSettingsPanel.nameLabels[slotIndex - 1].setText(
                "Slot " + juce::String(slotIndex) + ": " +
                    file.getFileNameWithoutExtension(),
                juce::dontSendNotification);
            // Save setting for next startup
            saveSetting("vsti_slot_" + juce::String(slotIndex),
                        file.getFullPathName());
          }
        });
      }
    });
  };

  vstiSettingsPanel.onRemoveVstiClicked = [this](int slotIndex) {
    karaokeEngine.getGraphManager().removeVstiPlugin(slotIndex);
    vstiSettingsPanel.nameLabels[slotIndex - 1].setText(
        "Slot " + juce::String(slotIndex) + ": Not Loaded",
        juce::dontSendNotification);
    saveSetting("vsti_slot_" + juce::String(slotIndex), "");
  };

  // VSTi plugins will be loaded by the Splash Screen before this component is
  // created. We just need to make sure the UI reflects the loaded state if
  // needed, but SettingsComponent handles its own labels when opened.
  {
    for (int i = 1; i <= 8; ++i) {
      juce::String savedPath = loadSetting("vsti_slot_" + juce::String(i));
      if (savedPath.isNotEmpty()) {
        juce::File vstiFile(savedPath);
        if (vstiFile.existsAsFile()) {
          vstiSettingsPanel.nameLabels[i - 1].setText(
              "Slot " + juce::String(i) + ": " +
                  vstiFile.getFileNameWithoutExtension(),
              juce::dontSendNotification);
        }
      }
    }
  }

  juce::Logger::writeToLog("MainComponent: Loading Database");
  juce::String dbPath = loadSetting("lastDbPath");

  // Set up Scanner Callbacks
  libraryScanner.onStatusChanged = [this](juce::String status) {
    juce::MessageManager::callAsync([this, status]() {
      // We can add a generic status label later, or just show it via debug
      // For now, let's keep it simple
    });
  };

  libraryScanner.onProgressChanged = [this](int percent) {
    juce::MessageManager::callAsync([this, percent]() {
      settingsComponent.setDbProgress(percent / 100.0);
    });
  };

  libraryScanner.onScanFinished = [this]() {
    juce::MessageManager::callAsync([this]() {
      settingsComponent.setTotalSongs(dbManager.getTotalCount());
      songListView.refreshList();
    });
  };

  // Start the timer to track mouse hover (100ms interval)
  startTimer(100);

  // Setup Bottom Bar
  bottomBar = std::make_unique<BottomBarComponent>();
  bottomBar->setLookAndFeel(&lookAndFeel);
  addAndMakeVisible(bottomBar.get());

  bottomBar->onPlayClicked = [this]() { karaokeEngine.play(); };
  bottomBar->onStopClicked = [this]() { karaokeEngine.stop(); };
  bottomBar->onNextClicked = [this]() {};

  bottomBar->onSeekChanged = [this](double normalizedPosition) {
    if (karaokeEngine.getMidiPlayer().isPlaying()) {
      double totalLengthSeconds = karaokeEngine.getMidiPlayer().getDuration();
      if (totalLengthSeconds > 0) {
        double newTime = normalizedPosition * totalLengthSeconds;
        karaokeEngine.getMidiPlayer().setPosition(newTime);
      }
    }
  };

  bottomBar->onMixerClicked = [this]() {
    if (mixerWindow == nullptr)
      mixerWindow = std::make_unique<SynthMixerWindow>(
          "Synth Mixer", karaokeEngine.getMixerController(),
          karaokeEngine.getGraphManager());

    mixerWindow->setVisible(!mixerWindow->isVisible());
  };

  lyricsView.attachToPlayer(&karaokeEngine.getMidiPlayer());

  karaokeEngine.onSongLoaded = [this]() {
    juce::MessageManager::callAsync(
        [this]() { lyricsView.setLyrics(karaokeEngine.getCurrentLyrics()); });
  };

  // Initialize Database in the /Songs folder
  juce::File appDir =
      juce::File::getSpecialLocation(juce::File::currentExecutableFile)
          .getParentDirectory();
  juce::File songsDir = appDir.getChildFile("Songs");
  songsDir.createDirectory(); // Ensure the directory exists
  juce::File dbFile = songsDir.getChildFile("songs.db");

  if (dbManager.initialize(dbFile)) {
    songListView.setDatabaseManager(&dbManager);
    settingsComponent.setTotalSongs(dbManager.getTotalCount());
    DBG("Database OK, count: " + juce::String(dbManager.getTotalCount()));
  }

  juce::Logger::writeToLog("MainComponent: Loading previous paths");
  // ─── โหลด paths ที่บันทึกไว้ก่อนหน้า ───────────────────────────────────────
  {
    // SoundFont is loaded by Splash Screen. Just update UI.
    juce::String lastSF2 = loadSetting("lastSF2Path");
    if (lastSF2.isNotEmpty()) {
      juce::File sf2File(lastSF2);
      if (sf2File.existsAsFile()) {
        settingsComponent.setSf2ButtonText(sf2File.getFileName());
      }
    }

    juce::String lastNcn = loadSetting("lastNcnFolder");
    if (lastNcn.isNotEmpty()) {
      settingsComponent.setNcnPath(lastNcn);
    }
    juce::String lastPkm = loadSetting("lastPkmFolder");
    if (lastPkm.isNotEmpty()) {
      settingsComponent.setPkmPath(lastPkm);
    }

    juce::String lastBg = loadSetting("lastBgPath");
    if (lastBg.isNotEmpty()) {
      juce::File bgFile(lastBg);
      if (bgFile.existsAsFile()) {
        juce::Image autoImg = juce::ImageCache::getFromFile(bgFile);
        if (autoImg.isValid()) {
          lyricsView.setCustomBackgroundImage(autoImg);
          DBG("Auto-loaded BG: " + lastBg);
        }
      }
    }
  }

  // List callback
  songListView.onSongSelected = [this](const SongRecord &entry) {
    bool wasEmpty = karaokeEngine.getQueueManager().isEmpty();
    karaokeEngine.getQueueManager().addSong(entry);

    if (wasEmpty && !karaokeEngine.getMidiPlayer().isPlaying()) {
      karaokeEngine.play();
    }
  };

  // Listen to mouse events on this component and all children to handle
  // right-clicks globally
  addMouseListener(this, true);

  juce::Logger::writeToLog("MainComponent: Constructor complete");
}

MainComponent::~MainComponent() {
  stopTimer();
  appProps().saveIfNeeded(); // บันทึกก่อนปิด
  juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
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

  // --- Update Seek Slider ---
  if (bottomBar != nullptr) {
    auto &player = karaokeEngine.getMidiPlayer();
    if (player.isPlaying()) {
      double currentPos = player.getPosition();
      double totalLen = player.getDuration();
      if (totalLen > 0.0) {
        bottomBar->updateSeekPosition(currentPos / totalLen);
      } else {
        bottomBar->updateSeekPosition(0.0);
      }
    } else {
      bottomBar->updateSeekPosition(0.0);
    }
  }
}

//==============================================================================
void MainComponent::paint(juce::Graphics &g) {
  g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized() {
  auto area = getLocalBounds();

  int panelWidth = 350;
  songListBounds = juce::Rectangle<int>(0, 0, panelWidth, getHeight());
  queueBounds =
      juce::Rectangle<int>(getWidth() - panelWidth, 0, panelWidth, getHeight());
  bottomBarBounds = juce::Rectangle<int>(0, getHeight() - 40, getWidth(), 40);

  // For WebView2, it must be exactly the size of the container, no shrinking
  // But we subtract 40 from the bottom so the Bottom Bar is strictly underneath
  auto mainArea = area.withTrimmedBottom(40);
  youtubePlayer.setBounds(mainArea);
  lyricsView.setBounds(mainArea);
  lyricsView.toFront(false);

  bottomBar->setBounds(bottomBarBounds);

  // If popups are active during a resize (e.g., window restored), update their
  // bounds natively
  if (isSongListVisible) {
    showNativePopup(&songListView, songListBounds);
  }
  if (isQueueVisible) {
    showNativePopup(&queueComponent, queueBounds);
  }
}

void MainComponent::showNativePopup(juce::Component *comp,
                                    juce::Rectangle<int> bounds,
                                    bool isTemporary) {
  if (comp == nullptr)
    return;

  // Position it in global screen coordinates
  juce::Rectangle<int> screenBounds =
      bounds.translated(getScreenX(), getScreenY());

  if (comp->isOnDesktop()) {
    comp->setBounds(screenBounds);
  } else {
    int windowFlags = juce::ComponentPeer::windowIgnoresKeyPresses |
                      juce::ComponentPeer::windowHasDropShadow;
    if (isTemporary)
      windowFlags |= juce::ComponentPeer::windowIsTemporary;

    comp->addToDesktop(windowFlags);
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

void MainComponent::mouseDown(const juce::MouseEvent &event) {
  if (event.mods.isPopupMenu()) {
    juce::PopupMenu menu;
    menu.setLookAndFeel(&lookAndFeel);
    menu.addItem("Settings", [this]() {
      if (settingsWindow == nullptr) {
        settingsWindow = std::make_unique<SettingsWindowContainer>(
            "Settings",
            lookAndFeel.findColour(juce::ResizableWindow::backgroundColourId),
            juce::DocumentWindow::closeButton);
        settingsWindow->setLookAndFeel(&lookAndFeel);
        settingsWindow->setContentNonOwned(&settingsComponent, true);
        settingsWindow->centreWithSize(300, 300);
        settingsWindow->setUsingNativeTitleBar(false);
      }
      settingsWindow->setVisible(true);
      settingsWindow->toFront(true);
    });
    menu.addItem("VST Instruments", [this]() {
      if (vstiSettingsWindow == nullptr) {
        vstiSettingsWindow = std::make_unique<SettingsWindowContainer>(
            "VST Instruments",
            lookAndFeel.findColour(juce::ResizableWindow::backgroundColourId),
            juce::DocumentWindow::closeButton);
        vstiSettingsWindow->setLookAndFeel(&lookAndFeel);
        vstiSettingsWindow->setContentNonOwned(&vstiSettingsPanel, true);
        vstiSettingsWindow->centreWithSize(450, 420);
        vstiSettingsWindow->setUsingNativeTitleBar(false);
      }
      vstiSettingsWindow->setVisible(true);
      vstiSettingsWindow->toFront(true);
    });
    menu.addSeparator();
    menu.addItem("Exit", []() {
      juce::JUCEApplication::getInstance()->systemRequestedQuit();
    });

    menu.showMenuAsync(juce::PopupMenu::Options());
  }
}
