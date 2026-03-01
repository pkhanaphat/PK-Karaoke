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
                            const juce::Colour &, bool isMouseOver,
                            bool isMouseDown) override {
    auto bounds = button.getLocalBounds().toFloat().reduced(2);

    float r = bounds.getHeight() * 0.5f;

    juce::Colour colour = button.getToggleState() ? accent : bgPanel;

    if (isMouseOver)
      colour = bgHover;

    if (isMouseDown)
      colour = accent;

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
  }

  void drawPopupMenuItem(juce::Graphics &g, const juce::Rectangle<int> &area,
                         bool, bool, bool isHighlighted, bool, bool,
                         const juce::String &text, const juce::String &,
                         const juce::Drawable *,
                         const juce::Colour *) override {
    auto bg = isHighlighted ? bgHover : bgPanel;

    g.setColour(bg);
    g.fillRect(area);

    g.setColour(textMain);

    g.drawText(text, area.reduced(10, 0), juce::Justification::centredLeft);
  }

  //==========================================================
  // TAB
  //==========================================================

  void drawTabButton(juce::TabBarButton &button, juce::Graphics &g,
                     bool isMouseOver, bool) override {
    auto area = button.getLocalBounds();

    juce::Colour c = button.isFrontTab() ? accent : bgPanel;

    if (isMouseOver)
      c = bgHover;

    g.setColour(c);

    g.fillRoundedRectangle(area.toFloat().reduced(2), 6);

    g.setColour(textMain);

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
