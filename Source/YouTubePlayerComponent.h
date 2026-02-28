#pragma once

#include <JuceHeader.h>

class YouTubePlayerComponent : public juce::Component
{
public:
    YouTubePlayerComponent();
    ~YouTubePlayerComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void loadVideo (const juce::String& videoId);
    void play();
    void pause();
    void stop();
    void setVolume (int volume0to100);

private:
    juce::WebBrowserComponent webBrowser;
    
    // HTML wrapper to bridge JUCE and YouTube IFrame API
    juce::String getHtmlContent (const juce::String& videoId);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YouTubePlayerComponent)
};
