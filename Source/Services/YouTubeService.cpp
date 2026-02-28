#include "Services/YouTubeService.h"

YouTubeService::YouTubeService() {}

juce::Array<YouTubeVideoInfo> YouTubeService::search(const juce::String &query,
                                                     int maxResults) {
  juce::Array<YouTubeVideoInfo> results;

  if (apiKey.isNotEmpty()) {
    results = searchViaApi(query, maxResults);
  }

  if (results.isEmpty()) {
    results = searchViaScraping(query, maxResults);
  }

  if (results.isEmpty()) {
    results = getFallbackData();
  }

  return results;
}

juce::Array<YouTubeVideoInfo>
YouTubeService::searchViaApi(const juce::String &query, int maxResults) {
  juce::Array<YouTubeVideoInfo> results;

  juce::URL url("https://www.googleapis.com/youtube/v3/search");
  url = url.withParameter("part", "snippet")
            .withParameter("type", "video")
            .withParameter("videoCategoryId", "10") // Music category
            .withParameter("maxResults", juce::String(maxResults))
            .withParameter("q", query)
            .withParameter("key", apiKey);

  // Synchronous request
  if (auto stream = url.createInputStream(juce::URL::InputStreamOptions(
          juce::URL::ParameterHandling::inAddress))) {
    juce::String resultStr = stream->readEntireStreamAsString();
    auto parsed = juce::JSON::parse(resultStr);

    if (auto *items = parsed.getProperty("items", juce::var()).getArray()) {
      for (auto &item : *items) {
        YouTubeVideoInfo info;
        auto idObj = item.getProperty("id", juce::var());
        info.id = idObj.getProperty("videoId", "").toString();

        auto snippet = item.getProperty("snippet", juce::var());
        info.title = snippet.getProperty("title", "").toString();
        info.channelTitle = snippet.getProperty("channelTitle", "").toString();

        auto thumbnails = snippet.getProperty("thumbnails", juce::var());
        auto highThumb = thumbnails.getProperty("high", juce::var());
        info.thumbnailUrl = highThumb.getProperty("url", "").toString();

        // Note: YouTube Search API doesn't return duration. We'd need another
        // API call to 'videos' endpoint. Leaving empty for basic
        // implementation.
        info.duration = "";

        results.add(info);
      }
    }
  }

  return results;
}

juce::Array<YouTubeVideoInfo>
YouTubeService::searchViaScraping(const juce::String &query, int maxResults) {
  juce::Array<YouTubeVideoInfo> results;

  juce::URL url("https://www.youtube.com/results");
  url = url.withParameter("search_query", query);

  // Spoof a User-Agent so we get HTML instead of generic reject
  juce::StringArray headers;
  headers.add(
      "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
      "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");

  if (auto stream = url.createInputStream(
          juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
              .withExtraHeaders(headers.joinIntoString("\r\n")))) {
    juce::String html = stream->readEntireStreamAsString();

    // Very basic scraping: Look for ytInitialData
    int startIndex = html.indexOf("ytInitialData = ");
    if (startIndex != -1) {
      int endIndex = html.substring(startIndex).indexOf(";</script>");
      if (endIndex != -1) {
        endIndex += startIndex;
        int jsonStart = startIndex + 16;
        juce::String jsonStr = html.substring(jsonStart, endIndex);

        // Because parsing this massive JSON with Juce's var can be slow and
        // brittle, we will extract using simple string matching for now.

        int safetyLimit = 50000;
        int searchPos = 0;
        while (results.size() < maxResults && --safetyLimit > 0) {
          int videoIdPos =
              jsonStr.substring(searchPos).indexOf("\"videoId\":\"");
          if (videoIdPos == -1)
            break;
          videoIdPos += searchPos;

          int idStart = videoIdPos + 11;
          int idEnd = jsonStr.substring(idStart).indexOf("\"");
          if (idEnd == -1)
            break;
          idEnd += idStart;
          juce::String vId = jsonStr.substring(idStart, idEnd);

          searchPos = idEnd;

          // Prevent duplicates
          bool isDup = false;
          for (auto &r : results) {
            if (r.id == vId) {
              isDup = true;
              break;
            }
          }
          if (isDup || vId.length() != 11)
            continue;

          YouTubeVideoInfo info;
          info.id = vId;

          // Find title near it
          int titleStartPos = jsonStr.substring(searchPos).indexOf(
              "\"title\":{\"runs\":[{\"text\":\"");
          if (titleStartPos != -1 && titleStartPos < 1000) {
            titleStartPos += searchPos;
            int tStart = titleStartPos + 26;
            int tEnd = jsonStr.substring(tStart).indexOf("\"");
            if (tEnd != -1) {
              tEnd += tStart;
              info.title = jsonStr.substring(tStart, tEnd);

              // Basic unescape
              info.title =
                  info.title.replace("\\u0026", "&").replace("\\\"", "\"");
            }
          }

          results.add(info);
        }
      }
    }
  }

  return results;
}

juce::Array<YouTubeVideoInfo> YouTubeService::getFallbackData() {
  juce::Array<YouTubeVideoInfo> results;

  // Just a couple of dummy entries for testing UI
  results.add({"dQw4w9WgXcQ",
               "Rick Astley - Never Gonna Give You Up (Official Music Video)",
               "Rick Astley", "", "3:33"});
  results.add({"kJQP7kiw5Fk", "Luis Fonsi - Despacito ft. Daddy Yankee",
               "Luis Fonsi", "", "4:42"});

  return results;
}

