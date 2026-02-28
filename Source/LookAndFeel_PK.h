#pragma once

#include <JuceHeader.h>

class LookAndFeel_PK : public juce::LookAndFeel_V4
{
public:
    LookAndFeel_PK()
    {
        // Global Colors
        setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xff1a1a1a));
        setColour (juce::TextButton::buttonColourId, juce::Colour (0xff333333));
        setColour (juce::TextButton::buttonOnColourId, juce::Colours::orange);
        setColour (juce::TextButton::textColourOffId, juce::Colours::white);
        
        setColour (juce::Slider::thumbColourId, juce::Colours::orange);
        setColour (juce::Slider::trackColourId, juce::Colour (0xff444444));
        setColour (juce::Slider::rotarySliderFillColourId, juce::Colours::orange);
        
        setColour (juce::Label::textColourId, juce::Colours::white);
        
        setColour (juce::ListBox::backgroundColourId, juce::Colour (0xff222222));
        setColour (juce::TableListBox::backgroundColourId, juce::Colour (0xff222222));
        setColour (juce::TableHeaderComponent::backgroundColourId, juce::Colour (0xff333333));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override
    {
        auto cornerSize = 4.0f;
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);

        auto baseColour = backgroundColour;
        if (shouldDrawButtonAsHighlighted) baseColour = baseColour.brighter (0.1f);
        if (shouldDrawButtonAsDown)        baseColour = baseColour.darker (0.1f);

        g.setColour (baseColour);
        g.fillRoundedRectangle (bounds, cornerSize);

        g.setColour (button.findColour (juce::ComboBox::outlineColourId).withAlpha (0.1f));
        g.drawRoundedRectangle (bounds, cornerSize, 1.0f);
    }
    
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           const float startAngle, const float endAngle, juce::Slider& slider) override
    {
        auto radius = (float)juce::jmin (width / 2, height / 2) - 4.0f;
        auto centreX = (float)x + (float)width  * 0.5f;
        auto centreY = (float)y + (float)height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = startAngle + sliderPos * (endAngle - startAngle);

        // fill
        g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
        juce::Path p;
        p.addCentredArc (centreX, centreY, radius, radius, 0.0f, startAngle, angle, true);
        g.strokePath (p, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // pointer
        g.setColour (juce::Colours::white);
        juce::Path ptr;
        ptr.addRectangle (-1.5f, -radius, 3.0f, radius * 0.4f);
        g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (centreX, centreY));
    }
};
