#pragma once

#include "Audio/AudioGraphManager.h"
#include "Core/Routing/MixerController.h"
#include "UI/CubaseMeterComponent.h"
#include "UI/LookAndFeel_PK_Mixer.h"
#include <JuceHeader.h>

//==============================================================================
// MixerFaderLAF - Custom LookAndFeel for the professional fader
//==============================================================================
class MixerFaderLAF : public juce::LookAndFeel_V4 {
public:
  MixerFaderLAF() = default;

  void setFaderColor(juce::Colour c) { faderColor = c; }

  // Convert dB value to a slider position (0.0 - 1.0)
  // 0dB -> 0.80, -10dB -> 0.50, -20dB -> 0.30, +6dB -> 1.0, -99dB -> 0.0
  static float dbToNorm(float db) {
    if (db >= 0.0f) {
      // 0dB to +6dB: occupies 20% at top (0.80 to 1.0)
      return juce::jmap(db, 0.0f, 6.0f, 0.80f, 1.0f);
    } else if (db >= -10.0f) {
      // -10dB to 0dB: occupies 30% in middle (0.50 to 0.80)
      return juce::jmap(db, -10.0f, 0.0f, 0.50f, 0.80f);
    } else if (db >= -20.0f) {
      // -20dB to -10dB: occupies 20% (0.30 to 0.50)
      return juce::jmap(db, -20.0f, -10.0f, 0.30f, 0.50f);
    } else {
      // -99dB to -20dB: occupies 30% at bottom (0.0 to 0.30)
      return juce::jmap(db, -99.0f, -20.0f, 0.0f, 0.30f);
    }
  }

  // Convert a slider position (0.0-1.0) to dB
  static float normToDb(float norm) {
    if (norm >= 0.80f) {
      return juce::jmap(norm, 0.80f, 1.0f, 0.0f, 6.0f);
    } else if (norm >= 0.50f) {
      return juce::jmap(norm, 0.50f, 0.80f, -10.0f, 0.0f);
    } else if (norm >= 0.30f) {
      return juce::jmap(norm, 0.30f, 0.50f, -20.0f, -10.0f);
    } else {
      return juce::jmap(norm, 0.0f, 0.30f, -99.0f, -20.0f);
    }
  }

  // Convert dB to linear gain
  static float dbToGain(float db) {
    return db <= -99.0f ? 0.0f : juce::Decibels::decibelsToGain(db);
  }

  void drawLinearSlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float minSliderPos, float maxSliderPos,
                        const juce::Slider::SliderStyle,
                        juce::Slider &slider) override {
    auto trackArea = juce::Rectangle<float>(
        (float)x + (float)width * 0.5f - 2.0f, (float)y, 4.0f, (float)height);

    // Track groove
    g.setColour(juce::Colour(0xff0a0a0a));
    g.fillRoundedRectangle(trackArea, 2.0f);
    g.setColour(juce::Colour(0xff333333));
    g.drawRoundedRectangle(trackArea, 2.0f, 1.0f);

    // Draw tick marks
    struct TickMark {
      float db;
      int emphasisLevel; // 0 = tiny, 1 = normal, 2 = major
    };
    static const TickMark ticks[] = {
        {6.0f, 1},   {4.0f, 0},   {2.0f, 0},   {0.0f, 2},   {-2.0f, 0},
        {-4.0f, 0},  {-6.0f, 1},  {-8.0f, 0},  {-10.0f, 1}, {-12.0f, 0},
        {-15.0f, 0}, {-17.0f, 0}, {-20.0f, 1}, {-40.0f, 0}, {-60.0f, 0},
        {-70.0f, 0}, {-90.0f, 0}};

    for (auto &tick : ticks) {
      float trackTop = (float)y;
      float trackBottom = (float)y + (float)height;
      float trackHeight = trackBottom - trackTop;

      float ty = trackBottom - (dbToNorm(tick.db) * trackHeight);

      float tw = 0;
      float th = 1.0f;
      juce::Colour c = juce::Colour(0xff4a4a4a);

      // Calculate track's left edge
      int trackLeftEdge = x + width / 2 - 1; // From trackX definition

      if (tick.emphasisLevel == 2) {
        tw = 12.0f;
        th = 1.5f;
        c = juce::Colour(0xffb0b0b0);
      } else if (tick.emphasisLevel == 1) {
        tw = 8.0f;
        c = juce::Colour(0xff808080);
      } else {
        tw = 5.0f;
      }

      g.setColour(c);

      // Right-aligned to the track, separated by a 4px gap
      int tickRight = trackLeftEdge - 4;
      int tickLeft = tickRight - (int)tw;
      int tickY = juce::roundToInt(ty);

      g.fillRect(juce::Rectangle<int>(tickLeft, tickY, (int)tw, (int)th));
    }

    // Fader thumb - compact metallic style
    float thumbH = 14.0f;                // smaller thumb
    float thumbW = (float)width * 0.55f; // narrowed thumb width
    float thumbX = (float)x + ((float)width - thumbW) * 0.5f;
    float thumbY = sliderPos - thumbH * 0.5f;

    // Shadow
    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.fillRoundedRectangle(thumbX + 1.5f, thumbY + 1.5f, thumbW, thumbH, 2.0f);

    // Main thumb body
    juce::Colour baseColor = faderColor;
    juce::Colour darkColor = baseColor.withMultipliedBrightness(0.55f);
    juce::ColourGradient thumbGrad(baseColor, thumbX, thumbY, darkColor, thumbX,
                                   thumbY + thumbH, false);
    thumbGrad.addColour(0.4, baseColor.brighter(0.2f));
    g.setGradientFill(thumbGrad);
    g.fillRoundedRectangle(thumbX, thumbY, thumbW, thumbH, 2.0f);

    // Top highlight
    g.setColour(juce::Colours::white.withAlpha(0.30f));
    g.fillRoundedRectangle(thumbX + 0.5f, thumbY + 0.5f, thumbW - 1.0f,
                           thumbH * 0.40f, 1.0f);

    // Center line
    g.setColour(juce::Colour(0xff333333));
    g.fillRect(juce::Rectangle<float>(
        thumbX + 3.0f, thumbY + thumbH * 0.5f - 0.5f, thumbW - 6.0f, 1.0f));

    // Outline
    g.setColour(juce::Colour(0xff111111));
    g.drawRoundedRectangle(thumbX, thumbY, thumbW, thumbH, 2.0f, 0.8f);
  }

private:
  juce::Colour faderColor{0xff999999}; // Default metallic grey
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerFaderLAF)
};

//==============================================================================
// ChannelStripComponent - Professional channel strip
//==============================================================================
class ChannelStripComponent : public juce::Component,
                              public juce::Slider::Listener,
                              public juce::Button::Listener,
                              public juce::ComboBox::Listener,
                              public juce::Timer {
public:
  ChannelStripComponent(MixerController &mc, AudioGraphManager &agm,
                        InstrumentGroup group, const juce::String &name);
  ~ChannelStripComponent() override;

  void paint(juce::Graphics &) override;
  void paintOverChildren(juce::Graphics &) override;
  void resized() override;
  void timerCallback() override;

  void sliderValueChanged(juce::Slider *slider) override;
  void buttonClicked(juce::Button *button) override;
  void comboBoxChanged(juce::ComboBox *comboBoxThatHasChanged) override;

  void updateStateFromController();

  void setExpanded(bool expanded);

  InstrumentGroup getTrackGroup() const { return trackGroup; }

  LookAndFeel_PK_Mixer mixerLAF;
  MixerFaderLAF faderLAF;

private:
  MixerController &mixerController;
  AudioGraphManager &audioGraphManager;
  InstrumentGroup trackGroup;
  juce::Label nameLabel;

  juce::Slider gainKnob;
  juce::Slider transposeScale;
  juce::TextButton muteButton;
  juce::TextButton soloButton;
  juce::TextButton insertSlots[4]; // Insert FX slots (VST)
  CubaseMeterComponent meter;
  juce::Slider volumeFader;
  juce::Slider auxSends[3];
  juce::Image instrumentIcon;
  juce::Rectangle<int> iconBounds;
  bool isExpanded = true;
  std::unique_ptr<juce::FileChooser> pluginChooser;

  void loadIcon();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelStripComponent)
};

//==============================================================================
// FXStripComponent - FX Return strip
//==============================================================================
class FXStripComponent : public juce::Component,
                         public juce::Slider::Listener,
                         public juce::Timer {
public:
  FXStripComponent(MixerController &mc, AudioGraphManager &agm,
                   InstrumentGroup group, const juce::String &name);
  ~FXStripComponent() override;

  void paint(juce::Graphics &) override;
  void paintOverChildren(juce::Graphics &) override;
  void resized() override;
  void timerCallback() override;
  void sliderValueChanged(juce::Slider *slider) override;

  void setExpanded(bool expanded);

  LookAndFeel_PK_Mixer mixerLAF;
  MixerFaderLAF faderLAF;

private:
  MixerController &mixerController;
  AudioGraphManager &audioGraphManager;
  InstrumentGroup fxGroup;
  juce::String fxName;

  juce::Label nameLabel;
  juce::TextButton insertSlots[4];
  CubaseMeterComponent meter;
  juce::Slider returnVolumeFader;
  juce::Image instrumentIcon;
  juce::Rectangle<int> iconBounds;
  bool isExpanded = true;
  std::unique_ptr<juce::FileChooser> pluginChooser;

  void loadIcon();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FXStripComponent)
};

//==============================================================================
// VstiStripComponent - Aggregates audio for a specific VSTi slot (1-8)
//==============================================================================
class VstiStripComponent : public juce::Component,
                           public juce::Slider::Listener,
                           public juce::Button::Listener,
                           public juce::Timer {
public:
  VstiStripComponent(MixerController &mc, AudioGraphManager &agm, int slot);
  ~VstiStripComponent() override;

  void paint(juce::Graphics &) override;
  void paintOverChildren(juce::Graphics &) override;
  void resized() override;
  void timerCallback() override;
  void sliderValueChanged(juce::Slider *) override;
  void buttonClicked(juce::Button *) override;
  void setExpanded(bool expanded);
  void updateStateFromController();
  void updateNameFromGraph();

  MixerFaderLAF faderLAF;
  LookAndFeel_PK_Mixer mixerLAF;

private:
  MixerController &mixerController;
  AudioGraphManager &audioGraphManager;
  int slotIndex;

  juce::Label nameLabel;
  juce::TextButton insertSlots[4];
  juce::TextButton muteButton;
  juce::TextButton soloButton;
  CubaseMeterComponent meter;
  juce::Slider volumeFader;
  juce::Slider gainKnob;
  juce::Slider auxSends[3];
  bool isExpanded = true;

  juce::Image pluginIcon;
  juce::Rectangle<int> iconBounds;

  void loadIcon();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VstiStripComponent)
};

//==============================================================================
// MasterStripComponent - Final output strip
//==============================================================================
class MasterStripComponent : public juce::Component,
                             public juce::Slider::Listener,
                             public juce::Timer {
public:
  MasterStripComponent(MixerController &mc, AudioGraphManager &agm);
  ~MasterStripComponent() override;

  void paint(juce::Graphics &) override;
  void paintOverChildren(juce::Graphics &) override;
  void resized() override;
  void timerCallback() override;
  void sliderValueChanged(juce::Slider *slider) override;

  void setExpanded(bool expanded);

  LookAndFeel_PK_Mixer mixerLAF;
  MixerFaderLAF faderLAF;

private:
  MixerController &mixerController;
  AudioGraphManager &audioGraphManager;
  CubaseMeterComponent meterLeft;
  CubaseMeterComponent meterRight;
  juce::Label nameLabel;
  juce::TextButton insertSlots[4];
  juce::Slider masterFader;
  juce::Image instrumentIcon;
  juce::Rectangle<int> iconBounds;
  bool isExpanded = true;
  std::unique_ptr<juce::FileChooser> pluginChooser;

  void loadIcon();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterStripComponent)
};

//==============================================================================
// MixerComponent - Container for all strips
//==============================================================================
class MixerComponent : public juce::Component {
public:
  MixerComponent(MixerController &mc, AudioGraphManager &agm);
  ~MixerComponent() override;

  void paint(juce::Graphics &) override;
  void resized() override;
  void toggleLayout();
  void updateAllStrips();
  void updateStripVisibility();

private:
  bool isExpandedLayout = true;
  void setupHeader();
  MixerController &mixerController;
  AudioGraphManager &audioGraphManager;

  struct HeaderComponent : public juce::Component {
    HeaderComponent();
    void paint(juce::Graphics &) override;
    void resized() override;

    juce::Label genreLabel;
    juce::OwnedArray<juce::TextButton> genreButtons;
    juce::TextButton saveButton{"Save"};
    juce::TextButton layoutButton{"Layout"};
  };

  HeaderComponent header;

  struct ContentComponent : public juce::Component {
    ContentComponent(juce::OwnedArray<ChannelStripComponent> &strips);
    void resized() override;
    juce::OwnedArray<ChannelStripComponent> &trackStrips;
  };

  struct SideDocksComponent : public juce::Component {
    SideDocksComponent(juce::OwnedArray<FXStripComponent> &fxStrips,
                       MasterStripComponent &master);
    void resized() override;
    juce::OwnedArray<FXStripComponent> &fxStrips;
    MasterStripComponent &master;
  };

  juce::OwnedArray<VstiStripComponent> vstiStrips;
  juce::OwnedArray<ChannelStripComponent> trackStrips;
  juce::OwnedArray<FXStripComponent> fxStrips;
  MasterStripComponent masterStrip;
  ContentComponent content;
  SideDocksComponent sideDocks;
  juce::Viewport viewport;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerComponent)
};
