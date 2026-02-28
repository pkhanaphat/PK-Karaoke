#pragma once

#include <JuceHeader.h>
#include "sqlite3.h"

struct SongRecord
{
    juce::String id;
    juce::String title;
    juce::String artist;
    juce::String key;
    int tempo;
    juce::String path;
    juce::String lyrics;
};

class DatabaseManager
{
public:
    DatabaseManager();
    ~DatabaseManager();

    bool initialize (const juce::File& dbFile);
    void close();

    // Rebuilds the database by calling the provided callback for each valid file
    // The UI or Scanner will use this to feed data
    bool beginBulkInsert();
    bool insertSong (const SongRecord& song);
    bool commitBulkInsert();
    bool rollbackBulkInsert();

    // Search using FTS5 FTS Matches
    juce::Array<SongRecord> searchSongs (const juce::String& query, int limit = 100);
    
    // Gets total number of songs in the DB
    int getTotalCount();

private:
    sqlite3* db = nullptr;
    sqlite3_stmt* insertStmt = nullptr;
    
    // Create schema
    void createTables();
    
    SongRecord readRecordFromStatement (sqlite3_stmt* stmt);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DatabaseManager)
};
