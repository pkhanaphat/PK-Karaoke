#include "UI/YouTubePlayerComponent.h"

YouTubePlayerComponent::YouTubePlayerComponent()
    : webBrowser(juce::WebBrowserComponent::Options().withBackend(
          juce::WebBrowserComponent::Options::Backend::webview2)) {
  addAndMakeVisible(webBrowser);
}

YouTubePlayerComponent::~YouTubePlayerComponent() {}

void YouTubePlayerComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colours::black);
}

void YouTubePlayerComponent::resized() {
  webBrowser.setBounds(getLocalBounds());
}

void YouTubePlayerComponent::loadVideo(const juce::String &videoId) {
  juce::String html = getHtmlContent(videoId);
  webBrowser.goToURL("data:text/html;charset=utf-8," + html);
}

void YouTubePlayerComponent::play() {
  webBrowser.goToURL("javascript:player.playVideo();");
}

void YouTubePlayerComponent::pause() {
  webBrowser.goToURL("javascript:player.pauseVideo();");
}

void YouTubePlayerComponent::stop() {
  webBrowser.goToURL("javascript:player.stopVideo();");
}

void YouTubePlayerComponent::setVolume(int volume0to100) {
  webBrowser.goToURL("javascript:player.setVolume(" +
                     juce::String(volume0to100) + ");");
}

juce::String
YouTubePlayerComponent::getHtmlContent(const juce::String &videoId) {
  // A simple wrapper for the YouTube IFrame API
  juce::String html = R"(
<!DOCTYPE html>
<html>
<head>
    <style>
        body, html { width: 100%; height: 100%; margin: 0; padding: 0; background-color: black; overflow: hidden; }
        #player { width: 100%; height: 100%; border: none; }
    </style>
</head>
<body>
    <div id="player"></div>

    <script>
      var tag = document.createElement('script');
      tag.src = "https://www.youtube.com/iframe_api";
      var firstScriptTag = document.getElementsByTagName('script')[0];
      firstScriptTag.parentNode.insertBefore(tag, firstScriptTag);

      var player;
      function onYouTubeIframeAPIReady() {
        player = new YT.Player('player', {
          height: '100%',
          width: '100%',
          videoId: 'VIDEO_ID_PLACEHOLDER',
          playerVars: {
            'playsinline': 1,
            'controls': 0, // We control it via JUCE
            'rel': 0,
            'showinfo': 0,
            'fs': 0
          },
          events: {
            'onReady': onPlayerReady
          }
        });
      }

      function onPlayerReady(event) {
        event.target.playVideo();
      }
    </script>
</body>
</html>
)";

  return html.replace("VIDEO_ID_PLACEHOLDER", videoId);
}

