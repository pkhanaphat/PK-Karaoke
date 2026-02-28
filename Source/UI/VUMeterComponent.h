#pragma once

#include <JuceHeader.h>

class VUMeterComponent : public juce::Component, public juce::Timer {
public:
  VUMeterComponent() {
    startTimerHz(30); // 30 FPS for smooth metering
  }

  ~VUMeterComponent() override { stopTimer(); }

  void setLevel(float newLevel) {
    level = juce::jlimit(0.0f, 1.5f, newLevel);
    if (level > peakLevel) {
      peakLevel = level;
      peakHoldCounter = 0;
    }
  }

  void timerCallback() override {
    // Smooth decay for the level bar
    level *= 0.85f;
    if (level < 0.001f)
      level = 0.0f;

    // Peak hold decay
    if (peakHoldCounter < 20) {
      peakHoldCounter++;
    } else {
      peakLevel *= 0.95f;
      if (peakLevel < 0.001f)
        peakLevel = 0.0f;
    }

    repaint();
  }

  void paint(juce::Graphics &g) override {
    auto area = getLocalBounds().toFloat();

    // Background track
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRect(area);

    // Meter segments (Sonar style)
    auto barHeight = area.getHeight();
    auto fillHeight = barHeight * level;

    juce::ColourGradient gradient(juce::Colours::green, 0, barHeight,
                                  juce::Colours::red, 0, 0, false);
    gradient.addColour(0.7, juce::Colours::yellow);

    g.setGradientFill(gradient);
    g.fillRect(area.withTop(barHeight - fillHeight));

    // Peak Hold Line
    if (peakLevel > 0.01f) {
      g.setColour(juce::Colours::white);
      float peakY = barHeight - (barHeight * peakLevel);
      g.drawHorizontalLine((int)peakY, area.getX(), area.getRight());
    }

    // Scale Markings
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    for (float i = 0.25f; i < 1.0f; i += 0.25f) {
      float y = barHeight - (barHeight * i);
      g.drawHorizontalLine((int)y, area.getX(), area.getRight());
    }
  }

private:
  float level = 0.0f;
  float peakLevel = 0.0f;
  int peakHoldCounter = 0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VUMeterComponent)
};
