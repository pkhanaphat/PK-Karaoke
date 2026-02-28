#include "UI/LyricsView.h"

LyricsView::LyricsView() {
  // Use a font that supports Thai characters. Arial usually works on Windows.
  // Or you can specify "Tahoma" or "Leelawadee UI".
  lyricsFont = juce::Font("Tahoma", 32.0f, juce::Font::bold);
}

LyricsView::~LyricsView() { stopTimer(); }

void LyricsView::setLyrics(const SongLyrics &newLyrics) {
  currentLyrics = newLyrics;
  currentCursorIndex = 0;
  repaint();
}

void LyricsView::attachToPlayer(MidiPlayer *player) {
  attachedPlayer = player;
  if (attachedPlayer != nullptr)
    startTimerHz(60); // 60 FPS update for smooth sweeping
  else
    stopTimer();
}

void LyricsView::timerCallback() {
  if (attachedPlayer != nullptr) {
    double newTime = attachedPlayer->getPosition();
    if (std::abs(newTime - currentPositionSeconds) >
        0.01) // Only repaint if changed slightly
    {
      currentPositionSeconds = newTime;
      repaint();
    }
  }
}

void LyricsView::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff1e1e1e)); // Dark background

  if (currentLyrics.lines.empty()) {
    g.setColour(juce::Colours::grey);
    g.setFont(lyricsFont);
    g.drawText("No Lyrics Loaded", getLocalBounds(),
               juce::Justification::centred, false);
    return;
  }

  g.setFont(lyricsFont);

  // 1. Find the current absolute character index based on position
  int absoluteCharIndex = 0;
  while (absoluteCharIndex < (int)currentLyrics.cursors.size() &&
         currentPositionSeconds >= currentLyrics.cursors[absoluteCharIndex]) {
    absoluteCharIndex++;
  }

  // Decrease by 1 to get the 'currently playing' char rather than 'next'
  if (absoluteCharIndex > 0)
    absoluteCharIndex--;

  // 2. Map absolute character index to Line and Column
  int activeLine = 0;
  int activeCharInLine = 0;
  int charCounter = 0;

  for (int i = 0; i < (int)currentLyrics.lines.size(); ++i) {
    int lineLen = currentLyrics.lines[i].length();
    if (absoluteCharIndex >= charCounter &&
        absoluteCharIndex <= charCounter + lineLen) {
      activeLine = i;
      activeCharInLine = absoluteCharIndex - charCounter;
      break;
    }
    charCounter += lineLen + 1; // +1 for the newline
  }

  // 3. Render the current and next lines
  juce::Rectangle<int> topHalf =
      getLocalBounds().removeFromTop(getHeight() / 2).reduced(20);
  juce::Rectangle<int> bottomHalf =
      getLocalBounds().removeFromBottom(getHeight() / 2).reduced(20);

  // Display two lines at a time
  int line1 = (activeLine / 2) * 2;
  int line2 = line1 + 1;

  auto drawSweptLine = [&](const juce::String &text,
                           juce::Rectangle<int> bounds, int sweepPos,
                           bool isActive) {
    if (text.isEmpty())
      return;

    // Draw background (un-swept) text
    g.setColour(juce::Colours::white);
    g.drawText(text, bounds, juce::Justification::centred, true);

    if (isActive && sweepPos > 0) {
      int fullWidth = lyricsFont.getStringWidth(text);
      float ratio = (float)sweepPos / (float)text.length();
      ratio = juce::jlimit(0.0f, 1.0f, ratio);

      int sweepWidth = (int)(fullWidth * ratio);

      // Clip to draw the swept part in a different color
      juce::Graphics::ScopedSaveState save(g);
      g.reduceClipRegion(
          bounds.withX(bounds.getX() + bounds.getWidth() / 2 - fullWidth / 2)
              .withWidth(sweepWidth));

      g.setColour(juce::Colours::cyan);
      g.drawText(text, bounds, juce::Justification::centred, true);
    }
  };

  if (line1 < (int)currentLyrics.lines.size())
    drawSweptLine(currentLyrics.lines[line1], topHalf,
                  (activeLine == line1) ? activeCharInLine
                                        : (activeLine > line1 ? 999 : 0),
                  true);

  if (line2 < (int)currentLyrics.lines.size())
    drawSweptLine(currentLyrics.lines[line2], bottomHalf,
                  (activeLine == line2) ? activeCharInLine
                                        : (activeLine > line2 ? 999 : 0),
                  true);

  // Draw debugging time info
  g.setFont(juce::Font(16.0f));
  g.setColour(juce::Colours::yellow);
  g.drawText("Time: " + juce::String(currentPositionSeconds, 2) + "s",
             getLocalBounds().removeFromBottom(30),
             juce::Justification::centred, false);
}

void LyricsView::resized() {
  // Adjust font size based on component height
  float height = (float)getHeight() / 4.0f;
  lyricsFont.setHeight(juce::jlimit(16.0f, 72.0f, height));
}

