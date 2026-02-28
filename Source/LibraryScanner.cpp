#include "LibraryScanner.h"

LibraryScanner::LibraryScanner (DatabaseManager& dbManager)
    : juce::Thread ("LibraryScanner"), db (dbManager)
{
}

LibraryScanner::~LibraryScanner()
{
    stopThread (2000);
}

void LibraryScanner::startScan (const juce::File& ncnRootFolder)
{
    if (isThreadRunning())
        stopThread (2000);

    rootFolder = ncnRootFolder;
    startThread();
}

void LibraryScanner::run()
{
    if (onStatusChanged) onStatusChanged ("Scanning files...");

    juce::File songDir = rootFolder.getChildFile ("Song");
    if (! songDir.exists() || ! songDir.isDirectory())
        songDir = rootFolder; // Fallback to root

    juce::Array<juce::File> midFiles;
    // NCN usually stores MIDI in Song folder, sometimes nested A-Z folders
    songDir.findChildFiles (midFiles, juce::File::findFiles, true, "*.mid");
    
    // Also try uppercase extension
    songDir.findChildFiles (midFiles, juce::File::findFiles, true, "*.MID");

    if (midFiles.isEmpty())
    {
        if (onStatusChanged) onStatusChanged ("No MIDI files found.");
        if (onScanFinished) onScanFinished();
        return;
    }

    if (! db.beginBulkInsert())
    {
        if (onStatusChanged) onStatusChanged ("Database lock error.");
        if (onScanFinished) onScanFinished();
        return;
    }

    int total = midFiles.size();
    int processed = 0;

    for (int i = 0; i < total; ++i)
    {
        if (threadShouldExit())
        {
            db.rollbackBulkInsert();
            return;
        }

        juce::File midFile = midFiles[i];
        SongRecord record;
        record.id = midFile.getFileNameWithoutExtension();
        record.title = record.id;
        record.artist = "Unknown";
        record.key = "-";
        record.tempo = 120; // Default fallback
        record.path = midFile.getFullPathName();
        
        // Find LYR file (Try relative to 'Lyrics' folder first)
        juce::String pathStr = midFile.getFullPathName();
        juce::String lyrPathStr = pathStr.replace ("\\Song\\", "\\Lyrics\\").replace ("\\song\\", "\\lyrics\\");
        lyrPathStr = lyrPathStr.upToLastOccurrenceOf (".", false, false) + ".lyr";
        
        juce::File lyrFile (lyrPathStr);
        if (! lyrFile.existsAsFile())
        {
            lyrFile = midFile.withFileExtension (".lyr"); // Next to MIDI
            if (! lyrFile.existsAsFile())
                lyrFile = midFile.withFileExtension (".LYR");
        }

        if (lyrFile.existsAsFile())
        {
            juce::MemoryBlock data;
            lyrFile.loadFileAsData (data);
            juce::String utf8Text = decodeTis620 (data);
            
            juce::StringArray lines;
            lines.addLines (utf8Text);
            
            if (lines.size() > 0) record.title = lines[0].trim();
            if (lines.size() > 1) record.artist = lines[1].trim();
            if (lines.size() > 2) record.key = lines[2].trim();
            
            if (record.key.isEmpty()) record.key = "-";
            if (lines.size() > 0) record.lyrics = lines[0].substring (0, 100); // Save a quick preview for search
        }

        // Ideally here we parse the MIDI header for TEMPO.
        // For now, we leave as 120. We can implement a quick binary read later.

        db.insertSong (record);

        processed++;
        if (processed % 100 == 0)
        {
            int percent = (int)(((float)processed / total) * 100.0f);
            if (onProgressChanged)
            {
                juce::MessageManager::callAsync ([this, percent]() {
                    if (onProgressChanged) onProgressChanged (percent);
                });
            }
        }
    }

    db.commitBulkInsert();

    if (onStatusChanged)
    {
        juce::MessageManager::callAsync ([this]() {
            if (onStatusChanged) onStatusChanged ("Database Rebuild Complete.");
            if (onProgressChanged) onProgressChanged (100);
            if (onScanFinished) onScanFinished();
        });
    }
}

juce::String LibraryScanner::decodeTis620 (const juce::MemoryBlock& data)
{
    auto* bytes = static_cast<const uint8_t*>(data.getData());
    size_t size = data.getSize();
    juce::String result;
    result.preallocateBytes (size * 3); 
    for (size_t i = 0; i < size; ++i)
    {
        uint8_t b = bytes[i];
        if (b < 0x80)
        {
            if (b != '\r') result << juce::String::charToString ((juce::juce_wchar)b);
        }
        else if (b >= 0xA1 && b <= 0xFB)
        {
            // Valid TIS-620 range -> map to Thai Unicode block
            result << juce::String::charToString ((juce::juce_wchar)(b + 0x0D60));
        }
        else result << " "; // Or '?'
    }
    return result;
}
