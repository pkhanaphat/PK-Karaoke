#pragma once
#include <JuceHeader.h>

class LookAndFeel_PK_Mixer : public juce::LookAndFeel_V4 {
public:
  LookAndFeel_PK_Mixer() {
    accent = juce::Colour(0xFF4A90E2); // Cubase blue
    accentDim = juce::Colour(0xFF2D5FA3);

    bgMain = juce::Colour(0xFF1E1F24);
    bgChannel = juce::Colour(0xFF2B2E36);
    bgChannelSel = juce::Colour(0xFF323743);

    track = juce::Colour(0xFF3A3F4B);

    text = juce::Colour(0xFFD6D9E0);
    textDim = juce::Colour(0xFF9AA0A6);

    setColour(juce::Slider::thumbColourId, accent);
    setColour(juce::Slider::trackColourId, track);
    setColour(juce::ResizableWindow::backgroundColourId, bgChannel);
    setColour(juce::TextButton::buttonOnColourId, accent);
    setColour(juce::TextButton::buttonColourId, bgChannel);
  }

  //==================================================
  // CHANNEL BACKGROUND
  //==================================================

  void drawGroupComponentOutline(juce::Graphics &g, int w, int h,
                                 const juce::String &,
                                 const juce::Justification &,
                                 juce::GroupComponent &group) override {
    auto bounds = juce::Rectangle<float>(0, 0, w, h);

    bool selected = group.hasKeyboardFocus(true);

    g.setColour(selected ? bgChannelSel : bgChannel);
    g.fillRoundedRectangle(bounds, 4.0f);
  }

  //==================================================
  // FADER
  //==================================================

  void drawLinearSlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float minSliderPos, float maxSliderPos,
                        const juce::Slider::SliderStyle,
                        juce::Slider &slider) override {
    auto trackX = x + width / 2 - 1;
    juce::Rectangle<float> track((float)trackX, (float)y, 2.0f, (float)height);

    g.setColour(juce::Colour(18, 18, 18));
    g.fillRect(track);

    // === dB Tick Marks (Left side) ===
    auto normalRange = slider.getNormalisableRange();
    const auto range = normalRange.getRange();
    const float minDb = (float)range.getStart();
    const float maxDb = (float)range.getEnd();

    const auto isMajorDb = [](float db) {
      return db == 0.0f || db == -10.0f || db == -20.0f || db == -50.0f;
    };

    auto drawTick = [&](float db, bool major) {
      if (db < minDb || db > maxDb)
        return;

      float norm = (float)normalRange.convertTo0to1(db);
      float ty = minSliderPos + (maxSliderPos - minSliderPos) * (1.0f - norm);

      float markW = 5.0f;
      juce::Colour c(95, 95, 95);
      float thickness = 1.0f;

      if (major) {
        c = juce::Colour(140, 140, 140);
        thickness = 1.6f;
        if (db == 0.0f)
          markW = 14.0f;
        else if (db == -10.0f || db == -20.0f)
          markW = 10.0f;
        else
          markW = 8.0f;
      } else {
        if (db <= -60.0f)
          c = juce::Colour(70, 70, 70);
        markW = (db <= -50.0f) ? 4.0f : 5.0f;
      }

      const float x1 = (float)x + width / 2.0f - markW - 6.0f;
      const float x2 = (float)x + width / 2.0f - 6.0f;
      g.setColour(c);
      g.drawLine(x1, ty, x2, ty, thickness);
    };

    // Dense ticks like a pro DAW: emphasize 0 / -10 / -20 / -50.
    for (float db = 0.0f; db >= -50.0f; db -= 5.0f)
      drawTick(db, isMajorDb(db));

    // Optional lighter continuation below -50 if the slider range extends
    // further.
    for (float db = -55.0f; db >= minDb; db -= 5.0f)
      drawTick(db, isMajorDb(db) || (((int)std::abs(db)) % 10 == 0));

    juce::Rectangle<float> thumb((float)(x + width / 2 - 14), sliderPos - 6.0f,
                                 28.0f, 12.0f);

    g.setColour(juce::Colour(210, 210, 210));
    g.fillRect(thumb);

    g.setColour(juce::Colour(40, 40, 40));
    g.drawRect(thumb);
  }

  void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float startAngle, float endAngle,
                        juce::Slider &slider) override {
    auto r =
        juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height)
            .reduced(2.0f);
    auto cx = r.getCentreX();
    auto cy = r.getCentreY();
    auto radius = r.getWidth() / 2.0f;

    g.setColour(juce::Colour(45, 45, 48));
    g.fillEllipse(r);

    juce::Colour outlineColor =
        slider.findColour(juce::Slider::rotarySliderOutlineColourId);
    if (outlineColor.isTransparent())
      outlineColor = juce::Colour(90, 95, 105);
    g.setColour(outlineColor);
    g.drawEllipse(r, 1.5f);

    float angle = startAngle + sliderPos * (endAngle - startAngle);
    juce::Point<float> p(
        cx + std::cos(angle - juce::MathConstants<float>::halfPi) * radius *
                 0.7f,
        cy + std::sin(angle - juce::MathConstants<float>::halfPi) * radius *
                 0.7f);

    juce::Colour pointerColor =
        slider.findColour(juce::Slider::rotarySliderFillColourId);
    if (pointerColor.isTransparent())
      pointerColor = juce::Colour(120, 180, 255);
    g.setColour(pointerColor);
    g.drawLine(cx, cy, p.x, p.y, 2.0f);
  }

  //==================================================
  // BUTTON (Mute / Solo)
  //==================================================

  void drawButtonBackground(juce::Graphics &g, juce::Button &button,
                            const juce::Colour &baseColour, bool hover,
                            bool down) override {
    auto bounds = button.getLocalBounds().toFloat();
    juce::Colour colour = baseColour;

    if (hover)
      colour = colour.brighter(0.1f);

    if (down)
      colour = colour.darker(0.2f);

    g.setColour(colour);
    g.fillRoundedRectangle(bounds, 4.0f);
  }

  void drawButtonText(juce::Graphics &g, juce::TextButton &button, bool,
                      bool) override {
    g.setColour(button.findColour(button.getToggleState()
                                      ? juce::TextButton::textColourOnId
                                      : juce::TextButton::textColourOffId));

    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText(button.getButtonText(), button.getLocalBounds(),
               juce::Justification::centred);
  }

  //==================================================
  // SCROLLBAR - Thicker & easier to click
  //==================================================

  int getScrollbarButtonSize(juce::ScrollBar &) override { return 0; }

  void drawScrollbar(juce::Graphics &g, juce::ScrollBar &bar, int x, int y,
                     int width, int height, bool isScrollbarVertical,
                     int thumbStartPosition, int thumbSize, bool isMouseOver,
                     bool isMouseDown) override {
    // Track
    g.setColour(juce::Colour(30, 30, 35));
    g.fillRect(x, y, width, height);

    // Thumb
    auto thumbColour = juce::Colour(80, 90, 110);
    if (isMouseOver)
      thumbColour = thumbColour.brighter(0.2f);
    if (isMouseDown)
      thumbColour = thumbColour.brighter(0.4f);

    g.setColour(thumbColour);
    if (isScrollbarVertical)
      g.fillRoundedRectangle((float)x + 1, (float)thumbStartPosition + 1,
                             (float)width - 2, (float)thumbSize - 2, 4.0f);
    else
      g.fillRoundedRectangle((float)thumbStartPosition + 1, (float)y + 1,
                             (float)thumbSize - 2, (float)height - 2, 4.0f);
  }

  //==================================================
  // LABEL
  //==================================================

  void drawLabel(juce::Graphics &g, juce::Label &label) override {
    g.setColour(text);
    g.setFont(13.0f);
    g.drawText(label.getText(), label.getLocalBounds(),
               juce::Justification::centred);
  }

private:
  juce::Colour accent;
  juce::Colour accentDim;
  juce::Colour bgMain;
  juce::Colour bgChannel;
  juce::Colour bgChannelSel;
  juce::Colour track;
  juce::Colour text;
  juce::Colour textDim;
};
