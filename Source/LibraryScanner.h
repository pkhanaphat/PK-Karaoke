#pragma once

#include <JuceHeader.h>
#include "DatabaseManager.h"

class LibraryScanner : public juce::Thread
{
public:
    LibraryScanner (DatabaseManager& dbManager);
    ~LibraryScanner() override;

    void startScan (const juce::File& ncnRootFolder);

    // Callbacks for UI updates
    std::function<void(int)> onProgressChanged;
    std::function<void(juce::String)> onStatusChanged;
    std::function<void()> onScanFinished;

    void run() override;

private:
    DatabaseManager& db;
    juce::File rootFolder;

    juce::String decodeTis620 (const juce::MemoryBlock& data);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LibraryScanner)
};
