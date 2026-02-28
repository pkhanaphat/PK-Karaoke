#include "UI/BottomBarComponent.h"

BottomBarComponent::BottomBarComponent() {
  // Define Paths
  juce::Path playPath;
  playPath.addTriangle(0, 0, 10, 5, 0, 10);

  juce::Path stopPath;
  stopPath.addRectangle(0, 0, 10, 10);

  juce::Path nextPath;
  nextPath.addTriangle(0, 0, 8, 5, 0, 10);
  nextPath.addRectangle(10, 0, 2, 10);

  juce::Path settingsPath;
  settingsPath.addEllipse(0, 0, 10, 10);
  settingsPath.addEllipse(3, 3, 4, 4);
  settingsPath.addRectangle(4, 0, 2, 10);
  settingsPath.addRectangle(0, 4, 10, 2);
  settingsPath.setUsingNonZeroWinding(false);

  juce::Path mixerPath;
  mixerPath.addRectangle(2, 0, 1.5f, 10);
  mixerPath.addRectangle(6.5f, 0, 1.5f, 10);
  mixerPath.addEllipse(0, 6, 5, 5);
  mixerPath.addEllipse(4.5f, 1, 5, 5);

  // Initialize unique_ptr buttons
  playButton = std::make_unique<IconButton>("Play", playPath);
  stopButton = std::make_unique<IconButton>("Stop", stopPath);
  nextButton = std::make_unique<IconButton>("Next", nextPath);
  settingsButton = std::make_unique<IconButton>("Settings", settingsPath);
  mixerButton = std::make_unique<IconButton>("Mixer", mixerPath);

  addAndMakeVisible(*playButton);
  addAndMakeVisible(*stopButton);
  addAndMakeVisible(*nextButton);
  addAndMakeVisible(*settingsButton);
  addAndMakeVisible(*mixerButton);

  playButton->onClick = [this] {
    if (onPlayClicked)
      onPlayClicked();
  };
  stopButton->onClick = [this] {
    if (onStopClicked)
      onStopClicked();
  };
  nextButton->onClick = [this] {
    if (onNextClicked)
      onNextClicked();
  };
  settingsButton->onClick = [this] {
    if (onSettingsClicked)
      onSettingsClicked();
  };
  mixerButton->onClick = [this] {
    if (onMixerClicked)
      onMixerClicked();
  };
}

BottomBarComponent::~BottomBarComponent() {}

void BottomBarComponent::paint(juce::Graphics &g) {
  g.setColour(juce::Colour(0xff1A1A1B).withAlpha(0.95f));
  g.fillRoundedRectangle(getLocalBounds().toFloat(), 15.0f);

  g.setColour(juce::Colours::white.withAlpha(0.1f));
  g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 15.0f, 1.0f);
}

void BottomBarComponent::resized() {
  auto bottomArea = getLocalBounds().reduced(
      20, 15); // Add more padding to shrink buttons visually

  int btnWidth = 40; // Reduced from 50
  settingsButton->setBounds(bottomArea.removeFromLeft(btnWidth));
  mixerButton->setBounds(bottomArea.removeFromRight(btnWidth));

  // Center play/stop/next
  int centerX = bottomArea.getWidth() / 2 + bottomArea.getX();
  int centerGroupWidth = btnWidth * 3 + 20; // 3 buttons + spacing
  auto centerArea =
      juce::Rectangle<int>(centerX - centerGroupWidth / 2, bottomArea.getY(),
                           centerGroupWidth, bottomArea.getHeight());

  playButton->setBounds(centerArea.removeFromLeft(btnWidth));
  centerArea.removeFromLeft(10);
  stopButton->setBounds(centerArea.removeFromLeft(btnWidth));
  centerArea.removeFromLeft(10);
  nextButton->setBounds(centerArea.removeFromLeft(btnWidth));
}
