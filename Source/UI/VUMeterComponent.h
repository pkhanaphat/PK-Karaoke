#pragma once

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

    const int numSegments = 30; // Fewer segments for a cleaner look
    const float gap = 1.0f;     // 1px gap

    float totalHeight = (float)area.getHeight();
    float totalGaps = (numSegments - 1) * gap;
    float segmentHeight = (totalHeight - totalGaps) / numSegments;

    // Draw from bottom to top
    for (int i = 0; i < numSegments; ++i) {
      // Calculate continuous Y position
      float bottomY = totalHeight - (i * (segmentHeight + gap));
      float topY = bottomY - segmentHeight;

      // Calculate normalized threshold for this segment
      float segmentThreshold = (i + 1) * (1.12f / (numSegments * 0.9f));

      bool isActive = level >= segmentThreshold;
      bool isPeak =
          peakLevel >= segmentThreshold &&
          peakLevel < segmentThreshold + (1.12f / (numSegments * 0.9f));

      juce::Colour segmentColour;
      int yellowThreshold = numSegments * 0.75;
      int redThreshold = numSegments * 0.9;

      if (i < yellowThreshold) {
        segmentColour = juce::Colour(0xff00cc00); // Green
      } else if (i < redThreshold) {
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

      g.fillRect(juce::Rectangle<float>((float)area.getX(), topY,
                                        (float)area.getWidth(), segmentHeight));
    }
  }

private:
  float level = 0.0f;
  float peakLevel = 0.0f;
  int peakHoldCounter = 0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VUMeterComponent)
};
