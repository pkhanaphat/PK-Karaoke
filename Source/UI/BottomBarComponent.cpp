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
  mixerButton = std::make_unique<IconButton>("Mixer", mixerPath);

  addAndMakeVisible(*playButton);
  addAndMakeVisible(*stopButton);
  addAndMakeVisible(*nextButton);
  addAndMakeVisible(*mixerButton);

  seekSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  seekSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  seekSlider.setRange(0.0, 1.0, 0.001);
  seekSlider.onValueChange = [this]() {
    // Only trigger seek if the user is dragging the thumb, not when updated
    // programmatically
    if (onSeekChanged && seekSlider.isMouseButtonDown())
      onSeekChanged(seekSlider.getValue());
  };
  addAndMakeVisible(seekSlider);

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
  mixerButton->onClick = [this] {
    if (onMixerClicked)
      onMixerClicked();
  };
}

BottomBarComponent::~BottomBarComponent() {}

void BottomBarComponent::paint(juce::Graphics &g) {
  g.setColour(juce::Colour(0xff333333).withAlpha(0.95f)); // Match bgSecondary
  g.fillRect(getLocalBounds().toFloat());

  // Removed outline
}

void BottomBarComponent::resized() {
  auto bottomArea =
      getLocalBounds().reduced(20, 8); // Adjust padding for smaller height

  int btnWidth = 24; // Scaled down button width

  // Mixer button goes to rightmost
  mixerButton->setBounds(bottomArea.removeFromRight(btnWidth));
  bottomArea.removeFromRight(15); // Add some spacing before the slider

  // Left-aligned playback buttons
  playButton->setBounds(bottomArea.removeFromLeft(btnWidth));
  bottomArea.removeFromLeft(10);
  stopButton->setBounds(bottomArea.removeFromLeft(btnWidth));
  bottomArea.removeFromLeft(10);
  nextButton->setBounds(bottomArea.removeFromLeft(btnWidth));
  bottomArea.removeFromLeft(20); // Spacing before slider starts

  // The rest of the area goes to the seek slider
  seekSlider.setBounds(bottomArea);
}

void BottomBarComponent::updateSeekPosition(double normalizedPosition) {
  // Only update if the user isn't actively dragging the slider
  if (!seekSlider.isMouseButtonDown()) {
    seekSlider.setValue(normalizedPosition, juce::dontSendNotification);
  }
}
