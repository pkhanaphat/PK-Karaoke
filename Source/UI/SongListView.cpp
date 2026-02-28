#include "UI/SongListView.h"

SongListView::SongListView() {
  thaiFont = juce::Font("Tahoma", 16.0f, juce::Font::plain);

  searchBox.addListener(this);
  searchBox.setTextToShowWhenEmpty("Search song or artist...",
                                   juce::Colours::grey);
  searchBox.setFont(thaiFont);
  addAndMakeVisible(searchBox);

  table.setModel(this);
  table.setColour(juce::ListBox::backgroundColourId,
                  juce::Colours::transparentBlack);
  table.setOutlineThickness(0);
  table.getHeader().addColumn("ID", 1, 80);
  table.getHeader().addColumn("Title", 2, 300);
  table.getHeader().addColumn("Artist", 3, 200);
  addAndMakeVisible(table);
}

SongListView::~SongListView() {}

void SongListView::setDatabaseManager(DatabaseManager *db) {
  dbManager = db;
  refreshList();
}

void SongListView::refreshList(const juce::String &query) {
  if (dbManager) {
    currentResults =
        dbManager->searchSongs(query, 200); // Limit to 200 for UI performance
  } else {
    currentResults.clear();
  }
  table.updateContent();
}

void SongListView::textEditorTextChanged(juce::TextEditor &editor) {
  juce::String query = editor.getText().trim();
  refreshList(query);
}

int SongListView::getNumRows() { return currentResults.size(); }

void SongListView::paintRowBackground(juce::Graphics &g, int rowNumber,
                                      int width, int height,
                                      bool rowIsSelected) {
  if (rowIsSelected) {
    auto selectionRect = juce::Rectangle<int>(width, height).reduced(4, 2);
    g.setColour(juce::Colour(0xffA100FF)
                    .withAlpha(0.2f)); // Match the new Electric Purple accent
    g.fillRoundedRectangle(selectionRect.toFloat(), 4.0f);
  } else if (rowNumber % 2 != 0) {
    g.setColour(juce::Colours::white.withAlpha(0.02f));
    g.fillRect(0, 0, width, height);
  }
}

void SongListView::paintCell(juce::Graphics &g, int rowNumber, int columnId,
                             int width, int height, bool rowIsSelected) {
  if (rowNumber >= currentResults.size())
    return;

  g.setColour(juce::Colours::white);
  g.setFont(thaiFont);

  const auto &song = currentResults[rowNumber];
  juce::String text;

  if (columnId == 1)
    text = song.id;
  else if (columnId == 2)
    text = song.title;
  else if (columnId == 3)
    text = song.artist;

  g.drawText(text, 4, 0, width - 8, height, juce::Justification::centredLeft,
             true);
}

void SongListView::cellDoubleClicked(int rowNumber, int columnId,
                                     const juce::MouseEvent &) {
  if (rowNumber < currentResults.size() && onSongSelected != nullptr) {
    onSongSelected(currentResults[rowNumber]);
  }
}

void SongListView::paint(juce::Graphics &g) {
  g.setColour(
      juce::Colour(0xff1A1A1B).withAlpha(0.95f)); // Changed to Deep Charcoal
  g.fillRoundedRectangle(getLocalBounds().toFloat(), 15.0f);

  g.setColour(juce::Colours::white.withAlpha(0.1f));
  g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 15.0f, 1.0f);
}

void SongListView::resized() {
  auto area = getLocalBounds().reduced(10); // Added padding
  searchBox.setBounds(area.removeFromTop(30));
  area.removeFromTop(10); // Spacing between search and table
  table.setBounds(area);
}

// Re-using the logic from NcnParser for independence here
juce::String SongListView::decodeTis620(const juce::MemoryBlock &data) {
  auto *bytes = static_cast<const uint8_t *>(data.getData());
  size_t size = data.getSize();
  juce::String result;
  result.preallocateBytes(size * 3);
  for (size_t i = 0; i < size; ++i) {
    uint8_t b = bytes[i];
    if (b < 0x80) {
      if (b != '\r')
        result << juce::String::charToString((juce::juce_wchar)b);
    } else if (b >= 0xA1 && b <= 0xFB) {
      result << juce::String::charToString((juce::juce_wchar)(b + 0x0D60));
    } else
      result << " ";
  }
  return result;
}
