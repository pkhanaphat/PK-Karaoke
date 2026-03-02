#include "UI/SplashComponent.h"

static juce::PropertiesFile::Options getSplashSettingsOptions() {
  juce::PropertiesFile::Options opts;
  opts.applicationName = "PKKaraokePlayer";
  opts.filenameSuffix = ".settings";
  opts.osxLibrarySubFolder = "Application Support";
  return opts;
}

static juce::ApplicationProperties &splashProps() {
  static juce::ApplicationProperties props;
  static bool initialised = false;
  if (!initialised) {
    props.setStorageParameters(getSplashSettingsOptions());
    initialised = true;
  }
  return props;
}

juce::String SplashComponent::loadSetting(const juce::String &key) {
  if (auto *p = splashProps().getUserSettings())
    return p->getValue(key);
  return {};
}

SplashComponent::SplashComponent(KaraokeEngine &engine,
                                 std::function<void()> onFinishedCallback)
    : karaokeEngine(engine), onFinished(onFinishedCallback),
      progressBar(progress) {
  setSize(600, 400);

  statusLabel.setFont(juce::Font(24.0f));
  statusLabel.setJustificationType(juce::Justification::centred);
  statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
  addAndMakeVisible(statusLabel);

  addAndMakeVisible(progressBar);

  startTimer(50); // Tick every 50ms
}

SplashComponent::~SplashComponent() { stopTimer(); }

void SplashComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff111111)); // Dark background

  g.setColour(juce::Colours::white);
  g.setFont(juce::Font(48.0f, juce::Font::bold));
  g.drawText("PK Karaoke Player", getLocalBounds().withBottom(getHeight() / 2),
             juce::Justification::centredBottom, true);

  g.setColour(juce::Colours::grey);
  g.setFont(juce::Font(18.0f));
  g.drawText(
      "Professional VST Audio Engine",
      getLocalBounds().withTop(getHeight() / 2 + 10).withBottom(getHeight()),
      juce::Justification::centredTop, true);
}

void SplashComponent::resized() {
  statusLabel.setBounds(10, getHeight() - 80, getWidth() - 20, 30);
  progressBar.setBounds(50, getHeight() - 40, getWidth() - 100, 20);
}

void SplashComponent::timerCallback() {
  // State machine for splash screen
  // Even numbers (0, 2, 4...) = update UI
  // Odd numbers (1, 3, 5...) = do heavy work
  if (currentStep == 0) {
    currentStatus = "Initializing Core Engine...";
    statusLabel.setText(currentStatus, juce::dontSendNotification);
    progress = 0.1;
  } else if (currentStep == 1) {
    // Core engine initialized in Main.cpp already
  } else if (currentStep == 2) {
    currentStatus = "Loading SoundFont...";
    statusLabel.setText(currentStatus, juce::dontSendNotification);
    progress = 0.2;
  } else if (currentStep == 3) {
    juce::String sf2Path = loadSetting("lastSF2Path");
    if (sf2Path.isNotEmpty() && juce::File(sf2Path).existsAsFile()) {
      karaokeEngine.loadSoundFont(juce::File(sf2Path));
    }
  } else if (currentStep == 4) {
    // Load per-track custom SoundFonts
    std::vector<InstrumentGroup> allGroups = {
        InstrumentGroup::Piano,     InstrumentGroup::Bass,
        InstrumentGroup::Strings,   InstrumentGroup::Brass,
        InstrumentGroup::SynthLead, InstrumentGroup::Kick,
        InstrumentGroup::Snare,     InstrumentGroup::HiHat};

    for (auto group : allGroups) {
      juce::String customPath =
          loadSetting("sf2_track_" + juce::String((int)group));
      if (customPath.isNotEmpty() && juce::File(customPath).existsAsFile()) {
        karaokeEngine.getGraphManager().setTrackSoundFont(
            group, juce::File(customPath));
      }
    }
  } else if (currentStep >= 5 && currentStep <= 20) {
    int slot = (currentStep - 5) / 2 + 1;   // 1 to 8
    bool isUiStep = (currentStep % 2 == 1); // 5, 7, 9... are UI steps

    if (isUiStep) {
      currentStatus = "Loading VSTi Slot " + juce::String(slot) + "...";
      statusLabel.setText(currentStatus, juce::dontSendNotification);
      progress = 0.2 + (0.05f * slot);
    } else {
      juce::String vstiPath = loadSetting("vsti_slot_" + juce::String(slot));
      if (vstiPath.isNotEmpty() && juce::File(vstiPath).existsAsFile()) {
        karaokeEngine.getGraphManager().loadVstiPlugin(slot, vstiPath);
      }
    }
  } else if (currentStep == 21) {
    currentStatus = "Ready!";
    statusLabel.setText(currentStatus, juce::dontSendNotification);
    progress = 1.0;
  } else if (currentStep == 22) {
    // Short delay
  } else if (currentStep == 23) {
    stopTimer();
    if (onFinished) {
      onFinished();
    }
    return;
  }

  currentStep++;
}
