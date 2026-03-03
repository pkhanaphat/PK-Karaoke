#pragma once
#include "BinaryData.h"
#include <JuceHeader.h>

class LookAndFeel_PK_Modern : public juce::LookAndFeel_V4 {
public:
  LookAndFeel_PK_Modern() {
    // FONT
    typeface = juce::Typeface::createSystemTypefaceFor(
        BinaryData::tahoma_ttf, BinaryData::tahoma_ttfSize);

    if (typeface)
      setDefaultSansSerifTypeface(typeface);

    // COLORS
    bgMain = juce::Colour(0xff1C1C1C);
    bgPanel = juce::Colour(0xff25282C);
    bgHover = juce::Colour(0xff2F3338);

    accent = juce::Colour(0xffFF8C00);

    textMain = juce::Colours::white;
    textDim = juce::Colour(0xffAAAAAA);

    // GLOBAL
    setColour(juce::ResizableWindow::backgroundColourId, bgMain);

    setColour(juce::PopupMenu::backgroundColourId, bgPanel);

    setColour(juce::TextButton::buttonColourId, bgPanel);
    setColour(juce::TextButton::buttonOnColourId, accent);

    setColour(juce::Label::textColourId, textMain);

    setColour(juce::Slider::thumbColourId, accent);

    setColour(juce::Slider::trackColourId, juce::Colour(0xff444444));

    setColour(juce::ListBox::backgroundColourId, bgMain);

    setColour(juce::ScrollBar::thumbColourId, accent.withAlpha(0.6f));
  }

  //==========================================================
  // FONT
  //==========================================================

  juce::Typeface::Ptr getTypefaceForFont(const juce::Font &) override {
    return typeface ? typeface : LookAndFeel_V4::getTypefaceForFont({});
  }

  //==========================================================
  // BUTTON
  //==========================================================

  void drawButtonBackground(juce::Graphics &g, juce::Button &button,
                            const juce::Colour &backgroundColour,
                            bool isMouseOver, bool isMouseDown) override {
    auto bounds = button.getLocalBounds().toFloat().reduced(2);
    float r = bounds.getHeight() * 0.5f;

    // Use the colour registered on the button itself (set via setColour()),
    // falling back to the global LookAndFeel colours only when not overridden.
    juce::Colour base = button.findColour(juce::TextButton::buttonColourId);
    juce::Colour down = button.findColour(juce::TextButton::buttonOnColourId);

    // Determine whether the button has a custom colour or uses the global
    // default
    bool hasCustomColour =
        (base != findColour(juce::TextButton::buttonColourId));

    juce::Colour colour;
    if (button.getToggleState()) {
      colour = down;
    } else {
      colour = base;
    }

    if (hasCustomColour) {
      // Custom-coloured buttons: lighten slightly on hover/down for subtle
      // feedback
      if (isMouseOver)
        colour = colour.brighter(0.12f);
      if (isMouseDown)
        colour = colour.brighter(0.22f);
    } else {
      // Default LookAndFeel_PK behaviour for non-custom buttons
      if (isMouseOver)
        colour = bgHover;
      if (isMouseDown)
        colour = accent;
    }

    g.setColour(colour);
    g.fillRoundedRectangle(bounds, r);
  }

  //==========================================================
  // SLIDER
  //==========================================================

  void drawLinearSlider(juce::Graphics &g, int x, int y, int w, int h,
                        float sliderPos, float minSliderPos, float maxSliderPos,
                        const juce::Slider::SliderStyle style,
                        juce::Slider &slider) override {
    if (style != juce::Slider::LinearHorizontal) {
      LookAndFeel_V4::drawLinearSlider(g, x, y, w, h, sliderPos, minSliderPos,
                                       maxSliderPos, style, slider);
      return;
    }

    auto track = juce::Rectangle<float>(x, y + h * 0.5f - 2, w, 4);

    g.setColour(juce::Colour(0xff444444));
    g.fillRoundedRectangle(track, 2);

    auto fill = track.withWidth(sliderPos - x);

    g.setColour(accent);
    g.fillRoundedRectangle(fill, 2);

    g.fillEllipse(sliderPos - 6, y + h * 0.5f - 6, 12, 12);
  }

  void drawRotarySlider(juce::Graphics &g, int x, int y, int w, int h,
                        float pos, float start, float end,
                        juce::Slider &slider) override {
    float radius = juce::jmin(w, h) / 2 - 6;

    float cx = x + w * 0.5f;
    float cy = y + h * 0.5f;

    float angle = start + pos * (end - start);

    // background arc
    g.setColour(juce::Colour(0xff444444));
    juce::Path bg;
    bg.addCentredArc(cx, cy, radius, radius, 0, start, end, true);
    g.strokePath(bg, juce::PathStrokeType(4));

    // fill arc
    g.setColour(accent);
    juce::Path fill;
    fill.addCentredArc(cx, cy, radius, radius, 0, start, angle, true);
    g.strokePath(fill, juce::PathStrokeType(4));

    // pointer
    g.fillEllipse(cx + std::cos(angle) * radius - 3,
                  cy + std::sin(angle) * radius - 3, 6, 6);
  }

  //==========================================================
  // POPUP MENU (NO BORDER)
  //==========================================================

  void drawPopupMenuBackground(juce::Graphics &g, int width,
                               int height) override {
    g.setColour(bgPanel);
    g.fillRect(0, 0, width, height);
    // Subtle border
    g.setColour(bgHover);
    g.drawRect(0, 0, width, height, 1);
  }

  void drawPopupMenuItem(juce::Graphics &g, const juce::Rectangle<int> &area,
                         bool isSeparator, bool, bool isHighlighted, bool,
                         bool isTicked, const juce::String &text,
                         const juce::String &, const juce::Drawable *,
                         const juce::Colour *) override {
    if (isSeparator) {
      g.setColour(bgHover);
      g.fillRect(area.reduced(8, 0).withHeight(1).withY(area.getCentreY()));
      return;
    }

    if (isHighlighted) {
      g.setColour(accent.withAlpha(0.2f));
      g.fillRect(area.reduced(4, 2));
      g.setColour(accent);
      g.fillRect(area.reduced(4, 2).removeFromLeft(3));
    }

    g.setColour(isHighlighted ? textMain : textDim);
    g.setFont(juce::Font(14.0f));
    auto textArea = area.reduced(16, 0);
    if (isTicked) {
      g.setColour(accent);
      g.drawText(juce::String(juce::CharPointer_UTF8("\xe2\x9c\x93")),
                 textArea.removeFromLeft(20), juce::Justification::centred);
    }
    g.setColour(isHighlighted ? textMain : textDim);
    g.drawText(text, textArea, juce::Justification::centredLeft);
  }

  //==========================================================
  // TAB
  //==========================================================

  void drawTabButton(juce::TabBarButton &button, juce::Graphics &g,
                     bool isMouseOver, bool isMouseDown) override {
    auto area = button.getLocalBounds();
    bool isFront = button.isFrontTab();

    juce::Colour c = isFront ? accent : bgPanel;

    // Only apply hover/down tint on tabs that are NOT already selected.
    // This prevents the selected (orange) tab from flickering to gray on hover.
    if (!isFront) {
      if (isMouseOver)
        c = bgHover;
      if (isMouseDown)
        c = accent.withAlpha(0.7f);
    }

    g.setColour(c);
    g.fillRoundedRectangle(area.toFloat().reduced(2), 6);

    // Text colour: dark on orange (selected) for readability, dim for others
    g.setColour(isFront ? juce::Colour(0xff1a1000) : textDim);

    g.drawText(button.getButtonText(), area, juce::Justification::centred);
  }

  //==========================================================
  // CALLOUT BOX (SETTINGS WINDOW)
  //==========================================================

  void drawCallOutBoxBackground(juce::CallOutBox &, juce::Graphics &g,
                                const juce::Path &path,
                                juce::Image &) override {
    g.setColour(bgPanel);

    g.fillPath(path);
  }

  //==========================================================
  // SCROLLBAR
  //==========================================================

  void drawScrollbar(juce::Graphics &g, juce::ScrollBar &, int x, int y, int w,
                     int h, bool vertical, int thumbStart, int thumbSize, bool,
                     bool) override {
    g.setColour(bgPanel);
    g.fillRect(x, y, w, h);

    g.setColour(accent.withAlpha(0.7f));

    if (vertical)
      g.fillRoundedRectangle(x + 2, thumbStart, w - 4, thumbSize, 4);
    else
      g.fillRoundedRectangle(thumbStart, y + 2, thumbSize, h - 4, 4);
  }

  //==========================================================
  // DOCUMENT WINDOW
  //==========================================================

  void drawDocumentWindowTitleBar(juce::DocumentWindow &window,
                                  juce::Graphics &g, int width, int height,
                                  int titleSpaceX, int titleSpaceW,
                                  const juce::Image *icon,
                                  bool drawTitleTextOnLeft) override {
    g.fillAll(bgPanel);

    g.setColour(textMain);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText(window.getName(), 10, 0, width, height,
               juce::Justification::centredLeft);

    // Subtle bottom line
    g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.fillRect(0, height - 1, width, 1);
  }

  void drawResizableWindowBorder(juce::Graphics &g, int w, int h,
                                 const juce::BorderSize<int> &border,
                                 juce::ResizableWindow &) override {
    g.setColour(bgPanel);
    g.fillAll();

    // Subtle outline
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.drawRect(0, 0, w, h, 1);
  }

private:
  juce::Typeface::Ptr typeface;

  juce::Colour bgMain;
  juce::Colour bgPanel;
  juce::Colour bgHover;

  juce::Colour accent;

  juce::Colour textMain;
  juce::Colour textDim;
};
