#include "UI/LyricsView.h"

LyricsView::LyricsView() {
  // Load background image from binary data
  backgroundImage = juce::ImageCache::getFromMemory(BinaryData::bg_png,
                                                    BinaryData::bg_pngSize);

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
    int newTime = attachedPlayer->getPositionTicks();
    if (newTime != currentPositionTicks) {
      currentPositionTicks = newTime;
      repaint();
    }
  }
}

void LyricsView::paint(juce::Graphics &g) {
  // 1. Draw Background Image
  if (backgroundImage.isValid()) {
    g.drawImage(backgroundImage, getLocalBounds().toFloat());
  } else {
    g.fillAll(juce::Colours::black);
  }

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
         currentPositionTicks >= currentLyrics.cursors[absoluteCharIndex]) {
    absoluteCharIndex++;
  }

  // Find the current absolutely playing character
  // If we haven't even reached the first character, absoluteCharIndex remains 0
  // but we shouldn't sweep it yet.
  bool hasReachedFirstChar = false;
  if (!currentLyrics.cursors.empty() &&
      currentPositionTicks >= currentLyrics.cursors[0]) {
    hasReachedFirstChar = true;
    // Decrease by 1 to get the 'currently playing' char rather than 'next'
    if (absoluteCharIndex > 0)
      absoluteCharIndex--;
  } else {
    absoluteCharIndex = -1; // Haven't started yet
  }

  // Calculate fractional progress through the current character
  float charFraction = 0.0f;
  if (hasReachedFirstChar && absoluteCharIndex >= 0 &&
      absoluteCharIndex < (int)currentLyrics.cursors.size()) {
    int startTick = currentLyrics.cursors[absoluteCharIndex];
    int nextTick = startTick;
    if (absoluteCharIndex + 1 < (int)currentLyrics.cursors.size()) {
      nextTick = currentLyrics.cursors[absoluteCharIndex + 1];
    }

    if (nextTick > startTick && currentPositionTicks >= startTick) {
      charFraction =
          (float)(currentPositionTicks - startTick) / (nextTick - startTick);
      charFraction = juce::jlimit(0.0f, 1.0f, charFraction);
    }
  }

  // 2. Map absolute character index to Line and Column
  int activeLine = 0;
  int activeCharInLine = 0;
  int charCounter = 0;

  for (int i = 0; i < (int)currentLyrics.lines.size(); ++i) {
    int lineLen = currentLyrics.lines[i].length();

    if (absoluteCharIndex == -1) {
      // We haven't started. Everything is unswept.
      activeLine = 0;
      activeCharInLine = -1;
      break;
    } else if (absoluteCharIndex >= charCounter &&
               absoluteCharIndex <= charCounter + lineLen) {
      activeLine = i;
      activeCharInLine = absoluteCharIndex - charCounter;
      break;
    }
    charCounter += lineLen + 1; // +1 for the newline
  }

  // 3. Render the alternating rolling lines
  // Match HandyKaraoke exactly but wider gap: Line 1 Y = height - 280, Line 2 Y
  // = height - 130
  juce::Rectangle<int> topBounds = getLocalBounds()
                                       .withSizeKeepingCentre(getWidth(), 80)
                                       .withY(getHeight() - 280);
  juce::Rectangle<int> bottomBounds = getLocalBounds()
                                          .withSizeKeepingCentre(getWidth(), 80)
                                          .withY(getHeight() - 130);

  // If activeLine is even (0, 2, 4...)
  // Top row shows activeLine, Bottom row shows activeLine + 1
  // If activeLine is odd (1, 3, 5...)
  // Top row shows activeLine + 1, Bottom row shows activeLine
  int topIndex = (activeLine % 2 == 0) ? activeLine : (activeLine + 1);
  int bottomIndex = (activeLine % 2 != 0) ? activeLine : (activeLine + 1);

  auto drawSweptLine = [&](const juce::String &text,
                           juce::Rectangle<int> bounds, int sweepChars,
                           float fraction, bool isFullySwept) {
    if (text.isEmpty())
      return;

    // Draw background (un-swept) text
    g.setColour(juce::Colours::white);
    g.drawText(text, bounds, juce::Justification::centred, true);

    if (isFullySwept || sweepChars >= 0) {
      int fullWidth = lyricsFont.getStringWidth(text);

      int sweepWidth = fullWidth;
      if (!isFullySwept) {
        juce::String sweptSoFar = text.substring(0, sweepChars);
        int baseWidth = lyricsFont.getStringWidth(sweptSoFar);

        if (sweepChars < text.length()) {
          juce::String currentCharStr =
              text.substring(sweepChars, sweepChars + 1);
          int charWidth = lyricsFont.getStringWidth(currentCharStr);
          sweepWidth = baseWidth + (int)(charWidth * fraction);
        } else {
          sweepWidth = baseWidth;
        }
      }

      // Clip to draw the swept part in a different color
      juce::Graphics::ScopedSaveState save(g);
      g.reduceClipRegion(
          bounds.withX(bounds.getX() + bounds.getWidth() / 2 - fullWidth / 2)
              .withWidth(sweepWidth));

      g.setColour(juce::Colours::cyan);
      g.drawText(text, bounds, juce::Justification::centred, true);
    }
  };

  if (topIndex < (int)currentLyrics.lines.size()) {
    bool isFullySwept = topIndex < activeLine;
    int sweepAmount = (topIndex == activeLine) ? activeCharInLine : -1;
    drawSweptLine(currentLyrics.lines[topIndex], topBounds, sweepAmount,
                  charFraction, isFullySwept);
  }

  if (bottomIndex < (int)currentLyrics.lines.size()) {
    bool isFullySwept = bottomIndex < activeLine;
    int sweepAmount = (bottomIndex == activeLine) ? activeCharInLine : -1;
    drawSweptLine(currentLyrics.lines[bottomIndex], bottomBounds, sweepAmount,
                  charFraction, isFullySwept);
  }

  // Draw debugging time info
  g.setFont(juce::Font(16.0f));
  g.setColour(juce::Colours::yellow);
  g.drawText("Tick: " + juce::String(currentPositionTicks) + " | Time: " +
                 juce::String(attachedPlayer->getPosition(), 2) + "s",
             getLocalBounds().removeFromBottom(30),
             juce::Justification::centred, false);
}

void LyricsView::resized() {
  // Adjust font size based on component height
  float height = (float)getHeight() / 4.0f;
  lyricsFont.setHeight(juce::jlimit(16.0f, 72.0f, height));
}
