#include "Core/NcnParser.h"

NcnParser::NcnParser() {}

SongLyrics NcnParser::parse(const juce::File &lyrFile,
                            const juce::File &curFile, int midiResolution) {
  SongLyrics result;

  if (!lyrFile.existsAsFile())
    return result;

  // 1. Read Cursors (.cur) First
  std::vector<int> allCursors;
  if (curFile.existsAsFile()) {
    juce::MemoryBlock curData;
    curFile.loadFileAsData(curData);

    auto *data = static_cast<const uint8_t *>(curData.getData());
    size_t size = curData.getSize();

    for (size_t i = 0; i + 1 < size; i += 2) {
      uint8_t b1 = data[i];
      uint8_t b2 = data[i + 1];
      int val = b1 + (b2 << 8);
      // HandyKaraoke Formula (Tick based)
      // The cursor values assume a specific resolution (likely 24 PPQ base).
      // We must scale it to the actual MIDI file's resolution.
      int tick = val * midiResolution / 24;
      allCursors.push_back(tick);
    }
  }

  // 2. Read Lyrics (.lyr)
  juce::MemoryBlock lyrData;
  lyrFile.loadFileAsData(lyrData);

  // Convert TIS-620 to UTF-8 String
  juce::String utf8Lyrics = convertTis620ToUtf8(lyrData);

  // Split into lines
  juce::StringArray rawLines;
  rawLines.addLines(utf8Lyrics);

  int linesToSkip = 4; // Skip 4 header lines

  std::vector<int> processedCursors = allCursors;
  int currentCursorIndex = 0;

  for (int i = 0; i < rawLines.size(); ++i) {
    if (i < linesToSkip)
      continue;

    juce::String line = rawLines[i].trimEnd();
    result.lines.push_back(line);

    if (line.isEmpty()) {
      // Insert duplicate cursor for blank line
      if (currentCursorIndex < (int)processedCursors.size()) {
        processedCursors.insert(processedCursors.begin() + currentCursorIndex,
                                processedCursors[currentCursorIndex]);
      }
      currentCursorIndex++;
    } else {
      // Advance cursor by line length + 1 (for newline)
      // Note: juce::String::length() returns number of characters, which is
      // correct for UTF-8 here because HandyKaraoke counts TIS-620 chars (1
      // byte = 1 char).
      currentCursorIndex += line.length() + 1;
    }
  }

  result.cursors = std::move(processedCursors);

  return result;
}

juce::String NcnParser::convertTis620ToUtf8(const juce::MemoryBlock &data) {
  // TIS-620 to Unicode mapping
  // ASCII (0x00-0x7F) is identical.
  // Thai characters start at TIS-620 0xA1 which maps to Unicode 0x0E01.

  auto *bytes = static_cast<const uint8_t *>(data.getData());
  size_t size = data.getSize();

  juce::String result;
  // Pre-allocate to avoid reallocations
  result.preallocateBytes(size * 3);

  for (size_t i = 0; i < size; ++i) {
    uint8_t b = bytes[i];

    if (b < 0x80) {
      // ASCII
      if (b != '\r') // Strip CR, keep LF
        result << juce::String::charToString((juce::juce_wchar)b);
    } else if (b >= 0xA1 && b <= 0xFB) {
      // TIS-620 Thai Range
      // 0xA1 (TIS) = 0x0E01 (Unicode) -> Offset is 0x0E01 - 0xA1 = 0x0D60
      juce::juce_wchar unicodeChar = (juce::juce_wchar)(b + 0x0D60);
      result << juce::String::charToString(unicodeChar);
    } else {
      // Unmapped or control characters in 0x80-0xA0 range (usually ignored or
      // space)
      result << " ";
    }
  }

  return result;
}
