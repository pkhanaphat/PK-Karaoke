#pragma once
#include <JuceHeader.h>

class CubaseMeterComponent : public juce::Component {
public:
  CubaseMeterComponent() = default;

  float level = 0.0f; // 0.0 → 1.0
  float peak = 0.0f;

  void setLevel(float newLevel) {
    float target = juce::jlimit(0.0f, 1.0f, newLevel);

    // Fast attack, smooth fall
    if (target > level)
      level += (target - level) * 0.7f;
    else
      level += (target - level) * 0.15f;

    level = juce::jlimit(0.0f, 1.0f, level);

    // Peak follows rising level immediately
    if (level > peak)
      peak = level;

    repaint();
  }

  void decayPeak() {
    peak *= 0.985f; // Slightly faster than 0.99f
    peak = juce::jlimit(0.0f, 1.0f, peak);

    if (peak < level)
      peak = level;

    repaint(); // must repaint so peak line moves smoothly between setLevel()
               // calls
  }

  void paint(juce::Graphics &g) override {
    // Save original bounds BEFORE any modification
    const auto totalBounds = getLocalBounds();

    // === Background ===
    g.fillAll(juce::Colour(10, 10, 10));

    // === Gradient (defined over full height) ===
    juce::ColourGradient grad;
    grad.addColour(0.0, juce::Colour(0, 255, 100));
    grad.addColour(0.6, juce::Colour(255, 220, 0));
    grad.addColour(0.8, juce::Colour(255, 140, 0));
    grad.addColour(1.0, juce::Colour(255, 0, 0));
    grad.point1 = {0.0f, (float)totalBounds.getBottom()};
    grad.point2 = {0.0f, (float)totalBounds.getY()};

    g.setGradientFill(grad);

    // === Meter fill ===
    int fillH = juce::roundToInt((float)totalBounds.getHeight() * level);
    auto fill = totalBounds.withTop(totalBounds.getBottom() - fillH);
    if (fillH > 0)
      g.fillRect(fill);

    // === Gloss ===
    if (fillH > 0) {
      juce::ColourGradient gloss(
          juce::Colours::white.withAlpha(0.18f), 0, (float)fill.getY(),
          juce::Colours::transparentWhite, 0, (float)fill.getBottom(), false);
      g.setGradientFill(gloss);
      g.fillRect(fill.withWidth(fill.getWidth() / 2));
    }

    // === Peak hold line (calculated from ORIGINAL bounds, clamped to [0,1])
    // ===
    if (peak > 0.01f) {
      float clampedPeak = juce::jlimit(0.0f, 1.0f, peak);
      float peakY = (float)totalBounds.getBottom() -
                    (float)totalBounds.getHeight() * clampedPeak;
      peakY = juce::jlimit((float)totalBounds.getY(),
                           (float)totalBounds.getBottom() - 1.0f, peakY);
      g.setColour(juce::Colours::white.withAlpha(0.85f));
      g.drawLine((float)totalBounds.getX(), peakY,
                 (float)totalBounds.getRight(), peakY, 1.5f);
    }

    // === Clip indicator ===
    if (level >= 0.98f) {
      g.setColour(juce::Colours::red);
      g.fillRect(totalBounds.getX(), totalBounds.getY(), totalBounds.getWidth(),
                 3);
    }

    // === Border ===
    g.setColour(juce::Colour(40, 40, 40));
    g.drawRect(getLocalBounds(), 1);
  }
};
