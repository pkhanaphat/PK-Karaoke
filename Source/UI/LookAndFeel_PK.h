#pragma once

#include <JuceHeader.h>

class LookAndFeel_PK : public juce::LookAndFeel_V4 {
public:
  LookAndFeel_PK() {
    // --- Modern Deep Charcoal & Electric Purple Palette ---
    // Deep background (Charcoal Grey)
    juce::Colour bgBase(0xff1A1A1B);
    juce::Colour bgSecondary(0xff252526);
    juce::Colour bgTertiary(0xff2D2D30);

    // Modern Premium Accents (Purple / Violet)
    juce::Colour accentColor(0xffA100FF); // Electric Purple
    juce::Colour accentHover(0xffBF4DFF);
    juce::Colour textMain = juce::Colours::white;
    juce::Colour textMuted(0xff888888);

    // Window & Basic Component Backgrounds
    setColour(juce::ResizableWindow::backgroundColourId, bgBase);

    // ScrollBar Styling (CRITICAL: User saw blue scrollbars in screenshot)
    setColour(juce::ScrollBar::backgroundColourId,
              juce::Colours::transparentBlack);
    setColour(juce::ScrollBar::thumbColourId, accentColor.withAlpha(0.6f));
    setColour(juce::ScrollBar::trackColourId, juce::Colours::transparentBlack);

    // Buttons
    setColour(juce::TextButton::buttonColourId, bgSecondary);
    setColour(juce::TextButton::buttonOnColourId, accentColor);
    setColour(juce::TextButton::textColourOffId, textMain);
    setColour(juce::TextButton::textColourOnId, juce::Colours::white);

    // Sliders
    setColour(juce::Slider::thumbColourId, accentColor);
    setColour(juce::Slider::trackColourId, bgTertiary);
    setColour(juce::Slider::rotarySliderFillColourId, accentColor);

    // Labels & TextEditors (Search Box)
    setColour(juce::Label::textColourId, textMain);
    setColour(juce::TextEditor::backgroundColourId, bgTertiary);
    setColour(juce::TextEditor::textColourId, textMain);
    setColour(juce::TextEditor::outlineColourId,
              juce::Colours::transparentBlack);
    setColour(juce::TextEditor::focusedOutlineColourId,
              accentColor.withAlpha(0.5f));
    setColour(juce::TextEditor::highlightColourId, accentColor.withAlpha(0.3f));

    // ListBox / Tables (Pitch Black, no outlines)
    setColour(juce::ListBox::backgroundColourId, bgBase);
    setColour(juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TableListBox::backgroundColourId, bgBase);
    setColour(juce::TableListBox::outlineColourId,
              juce::Colours::transparentBlack);

    // Table Header (Pitch Black & Borderless)
    setColour(juce::TableHeaderComponent::backgroundColourId, bgBase);
    setColour(juce::TableHeaderComponent::textColourId, textMuted);
    setColour(juce::TableHeaderComponent::outlineColourId,
              juce::Colours::transparentBlack);
  }

  // =========================================================================
  // BUTTONS
  // =========================================================================
  void drawButtonBackground(juce::Graphics &g, juce::Button &button,
                            const juce::Colour &backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override {
    auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
    auto cornerSize = bounds.getHeight() * 0.5f; // Fully rounded pills

    juce::Colour baseColour = backgroundColour;

    if (shouldDrawButtonAsDown) {
      baseColour = baseColour.darker(0.2f);
    } else if (shouldDrawButtonAsHighlighted) {
      baseColour = baseColour.brighter(0.1f);
    }

    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, cornerSize);

    // Subtle inner glow/outline instead of harsh border
    if (!button.getToggleState()) {
      g.setColour(juce::Colours::white.withAlpha(0.05f));
      g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
    } else {
      g.setColour(button.findColour(juce::TextButton::buttonOnColourId)
                      .withAlpha(0.5f));
      g.drawRoundedRectangle(bounds.reduced(1.0f), cornerSize - 1.0f, 2.0f);
    }
  }

  // Note: drawListBoxItem is not a LookAndFeel method, it belongs to
  // ListBoxModel. The background selection colors are handled via
  // ListBox::backgroundColourId and ListBoxModel implementations.

  // =========================================================================
  // TABLE HEADERS (Clean, Modern Flat Look)
  // =========================================================================
  void drawTableHeaderColumn(juce::Graphics &g,
                             juce::TableHeaderComponent &header,
                             const juce::String &columnName, int /*columnId*/,
                             int width, int height, bool isMouseOver,
                             bool isMouseDown, int columnFlags) override {
    auto bounds = juce::Rectangle<int>(width, height);

    juce::Colour bg =
        header.findColour(juce::TableHeaderComponent::backgroundColourId);
    if (isMouseDown)
      bg = bg.darker(0.1f);
    else if (isMouseOver)
      bg = bg.brighter(0.05f);

    g.setColour(bg);
    g.fillRect(bounds);

    // Only draw a subtle bottom border, not side borders
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.fillRect(0, height - 1, width, 1);

    g.setColour(header.findColour(juce::TableHeaderComponent::textColourId));
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText(columnName, bounds.reduced(8, 0),
               juce::Justification::centredLeft, true);
  }

  // =========================================================================
  // ROTARY SLIDERS
  // =========================================================================
  void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, const float startAngle,
                        const float endAngle, juce::Slider &slider) override {
    auto radius = (float)juce::jmin(width / 2, height / 2) - 6.0f;
    auto centreX = (float)x + (float)width * 0.5f;
    auto centreY = (float)y + (float)height * 0.5f;
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = startAngle + sliderPos * (endAngle - startAngle);

    // Track (Background Ring)
    g.setColour(slider.findColour(juce::Slider::trackColourId));
    juce::Path pbg;
    pbg.addCentredArc(centreX, centreY, radius, radius, 0.0f, startAngle,
                      endAngle, true);
    g.strokePath(pbg, juce::PathStrokeType(6.0f, juce::PathStrokeType::mitered,
                                           juce::PathStrokeType::rounded));

    // Fill (Active Ring)
    g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId));
    juce::Path p;
    p.addCentredArc(centreX, centreY, radius, radius, 0.0f, startAngle, angle,
                    true);
    g.strokePath(p, juce::PathStrokeType(6.0f, juce::PathStrokeType::mitered,
                                         juce::PathStrokeType::rounded));

    // Pointer
    g.setColour(juce::Colours::white);
    juce::Path ptr;
    ptr.addEllipse(-3.0f, -radius - 3.0f, 6.0f,
                   6.0f); // Clean dot instead of rectangle
    g.fillPath(ptr, juce::AffineTransform::rotation(angle).translated(centreX,
                                                                      centreY));
  }
};
