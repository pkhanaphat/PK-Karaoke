cmake_minimum_required(VERSION 3.20)
project(PKKaraokeVST3 VERSION 1.0.0)

# Fetch JUCE automatically (using relatively recent stable version)
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4 # Latest stable as of mid 2024
)
FetchContent_MakeAvailable(juce)

# Create the executable target
juce_add_gui_app(PKKaraokeVST3
    PRODUCT_NAME "PK Karaoke Player"
    VERSION 1.0.0
    COMPANY_NAME "PK SOFT"
)

# Enforce C++20
target_compile_features(PKKaraokeVST3 PUBLIC cxx_std_20)

# Source files
target_sources(PKKaraokeVST3 PRIVATE
    Source/Main.cpp
    Source/MainComponent.h
    Source/MainComponent.cpp
    Source/NcnParser.h
    Source/NcnParser.cpp
    Source/SoundFontSynth.h
    Source/SoundFontSynth.cpp
    Source/MidiPlayer.h
    Source/MidiPlayer.cpp
    Source/AudioGraphManager.h
    Source/AudioGraphManager.cpp
    Source/MixerController.h
    Source/MixerController.cpp
    Source/MidiHelper.h
    Source/MidiHelper.cpp
    Source/KaraokeEngine.h
    Source/KaraokeEngine.cpp
    Source/YouTubeService.h
    Source/YouTubeService.cpp
    Source/YouTubePlayerComponent.h
    Source/YouTubePlayerComponent.cpp
    Source/LyricsView.h
    Source/LyricsView.cpp
    Source/MixerComponent.h
    Source/MixerComponent.cpp
    Source/SongListView.h
    Source/SongListView.cpp
    Source/sqlite3.c
    Source/DatabaseManager.h
    Source/DatabaseManager.cpp
    Source/LibraryScanner.h
    Source/LibraryScanner.cpp
    Source/QueueManager.h
    Source/QueueManager.cpp
)

# Enable VST3 Hosting and Web Browser
target_compile_definitions(PKKaraokeVST3 PUBLIC
    JUCE_PLUGINHOST_VST3=1
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=1
)

# Link against JUCE modules
target_link_libraries(PKKaraokeVST3 PRIVATE
    juce::juce_core
    juce::juce_events
    juce::juce_graphics
    juce::juce_data_structures
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_utils
    juce::juce_audio_processors
)

# Generate JuceHeader.h
juce_generate_juce_header(PKKaraokeVST3)
