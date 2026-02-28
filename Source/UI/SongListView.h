#pragma once

#include "Database/DatabaseManager.h"
#include <JuceHeader.h>


class SongListView : public juce::Component,
                     public juce::TableListBoxModel,
                     public juce::TextEditor::Listener {
public:
  SongListView();
  ~SongListView() override;

  void paint(juce::Graphics &) override;
  void resized() override;

  void setDatabaseManager(DatabaseManager *db);
  void refreshList(const juce::String &query = "");

  // std::function callback when a user double clicks a song
  std::function<void(const SongRecord &)> onSongSelected;

  // TableListBoxModel override
  int getNumRows() override;
  void paintRowBackground(juce::Graphics &g, int rowNumber, int width,
                          int height, bool rowIsSelected) override;
  void paintCell(juce::Graphics &g, int rowNumber, int columnId, int width,
                 int height, bool rowIsSelected) override;
  void cellDoubleClicked(int rowNumber, int columnId,
                         const juce::MouseEvent &) override;

  // TextEditor Listener
  void textEditorTextChanged(juce::TextEditor &editor) override;

  juce::String decodeTis620(const juce::MemoryBlock &data);

  juce::TextEditor searchBox;
  juce::TableListBox table;

  DatabaseManager *dbManager = nullptr;
  juce::Array<SongRecord> currentResults;

  juce::Font thaiFont;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SongListView)
};

