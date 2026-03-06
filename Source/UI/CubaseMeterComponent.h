#pragma once
#include <JuceHeader.h>

class CubaseMeterComponent : public juce::Component {
public:
  CubaseMeterComponent() {
    setOpaque(true);
  }

  float lastDrawnLevel = -1.0f;
  float lastDrawnPeak = -1.0f;

  float level = 0.0f; // 0.0 → 1.0
  float peak = 0.0f;

  void setLevel(float newLevel) {
    float target = juce::jlimit(0.0f, 1.0f, newLevel);

    // Sharper attack (0.85), more responsive fall (0.2)
    if (target > level)
      level += (target - level) * 0.85f;
    else
      level += (target - level) * 0.20f;

    level = juce::jlimit(0.0f, 1.0f, level);

    if (level > peak)
      peak = level;

    // Direct repaint since we are on 60Hz now and GPU is handling it
    if (std::abs(level - lastDrawnLevel) > 0.002f || std::abs(peak - lastDrawnPeak) > 0.002f) {
      lastDrawnLevel = level;
      lastDrawnPeak = peak;
      repaint();
    }
  }

  void decayPeak() {
    peak *= 0.98f; // Faster peak decay (0.98 instead of 0.985)
    peak = juce::jlimit(0.0f, 1.0f, peak);

    if (peak < level)
      peak = level;

    if (std::abs(level - lastDrawnLevel) > 0.002f || std::abs(peak - lastDrawnPeak) > 0.002f) {
      lastDrawnLevel = level;
      lastDrawnPeak = peak;
      repaint();
    }
  }

  void paint(juce::Graphics &g) override {
    const auto r = getLocalBounds();
    
    // Background
    g.fillAll(juce::Colour(5, 5, 5));

    const int numSegments = 30;
    const float spacing = 1.0f;
    const float totalH = (float)r.getHeight();
    const float segmentH = (totalH - (numSegments - 1) * spacing) / numSegments;

    for (int i = 0; i < numSegments; ++i) {
        // normalized height of this segment (bottom to top)
        float normY = (float)i / (float)numSegments;
        float yPos = totalH - (float)(i + 1) * (segmentH + spacing);
        
        juce::Rectangle<float> segRect((float)r.getX() + 1.0f, yPos, (float)r.getWidth() - 2.0f, segmentH);

        bool isActive = (level > normY && level > 1e-4f);
        bool isPeak = (peak > normY && peak < normY + (1.0f/numSegments) && peak > 1e-4f);

        if (isActive || isPeak) {
            juce::Colour c;
            if (normY < 0.6f) 
                c = juce::Colour(0, 220, 80); // Green
            else if (normY < 0.85f)
                c = juce::Colour(255, 200, 0); // Yellow
            else
                c = juce::Colour(255, 30, 0);  // Red
            
            if (!isActive && isPeak)
                g.setColour(c.withAlpha(0.9f));
            else
                g.setColour(c);

            g.fillRect(segRect);
        } else {
            // Unlit segment
            g.setColour(juce::Colour(30, 30, 35));
            g.fillRect(segRect);
        }
    }

    // Border
    g.setColour(juce::Colour(45, 45, 50));
    g.drawRect(r, 1);
  }
};
