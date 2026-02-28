#pragma once

#include <JuceHeader.h>

class VUMeterComponent : public juce::Component, public juce::Timer {
public:
  VUMeterComponent() {
    setOpaque(true);
    startTimerHz(60); // 60 FPS for high-end smooth metering
  }

  ~VUMeterComponent() override { stopTimer(); }

  void setLevel(float newLevel) {
    auto limitedLevel = juce::jlimit(0.0f, 1.5f, newLevel);

    // Low-pass filter for rising edge to make it smoother (+15% of new value
    // per frame)
    if (limitedLevel > level)
      level += (limitedLevel - level) * 0.4f;
    else
      level = limitedLevel;

    if (level > peakLevel) {
      peakLevel = level;
      peakHoldCounter = 0;
    }
  }

  void timerCallback() override {
    // Smoother falling decay for 60Hz (from 0.92f to 0.96f)
    level *= 0.96f;
    if (level < 0.001f)
      level = 0.0f;

    // Peak hold decay (from 0.96f to 0.98f)
    if (peakHoldCounter < 40) { // Hold for ~0.6s at 60Hz
      peakHoldCounter++;
    } else {
      peakLevel *= 0.98f;
      if (peakLevel < 0.001f)
        peakLevel = 0.0f;
    }

    repaint();
  }

  void paint(juce::Graphics &g) override {
    auto area = getLocalBounds();

    // Draw solid black background to prevent ghosting
    g.fillAll(juce::Colours::black);

    const int numSegments = 40;
    const int gap = 1; // 1px gap to fit 40 segments nicely

    float totalHeight = (float)area.getHeight();
    float segmentHeight = (totalHeight - (numSegments - 1) * gap) / numSegments;

    for (int i = 0; i < numSegments; ++i) {
      // Index 0 is bottom, numSegments-1 is top
      float yPos = totalHeight - (i + 1) * (segmentHeight + gap) + gap;

      // Calculate normalized threshold for this segment
      // 1.12 is approx +1dB. With 40 segments, the red zone (starts at 36)
      // begins at +1dB.
      float segmentThreshold = (i + 1) * (1.12f / 36.0f);

      bool isActive = level >= segmentThreshold;
      bool isPeak = peakLevel >= segmentThreshold &&
                    peakLevel < segmentThreshold + (1.12f / 36.0f);

      juce::Colour segmentColour;
      if (i < 30) {
        segmentColour = juce::Colours::green;
      } else if (i < 36) {
        segmentColour = juce::Colours::yellow;
      } else {
        segmentColour = juce::Colours::red;
      }

      if (isActive) {
        g.setColour(segmentColour);
      } else if (isPeak) {
        g.setColour(juce::Colours::white.withAlpha(0.6f));
      } else {
        g.setColour(segmentColour.withAlpha(0.12f));
      }

      g.fillRect(area.getX(), (int)yPos, area.getWidth(), (int)segmentHeight);
    }
  }

private:
  float level = 0.0f;
  float peakLevel = 0.0f;
  int peakHoldCounter = 0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VUMeterComponent)
};
