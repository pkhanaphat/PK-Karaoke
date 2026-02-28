#pragma once

#include <JuceHeader.h>

struct YouTubeVideoInfo
{
    juce::String id;
    juce::String title;
    juce::String channelTitle;
    juce::String thumbnailUrl;
    juce::String duration;
};

class YouTubeService
{
public:
    YouTubeService();
    ~YouTubeService() = default;

    // Set API Key if available
    void setApiKey (const juce::String& key) { apiKey = key; }

    // Performs search synchronously
    juce::Array<YouTubeVideoInfo> search (const juce::String& query, int maxResults = 10);

private:
    juce::String apiKey;

    juce::Array<YouTubeVideoInfo> searchViaApi (const juce::String& query, int maxResults);
    juce::Array<YouTubeVideoInfo> searchViaScraping (const juce::String& query, int maxResults);
    
    // Fallback data if everything fails (useful for immediate testing)
    juce::Array<YouTubeVideoInfo> getFallbackData();

    juce::String formatDuration (const juce::String& isoDuration);
};
