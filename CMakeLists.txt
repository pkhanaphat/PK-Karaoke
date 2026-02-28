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

    # UI Layer
    Source/UI/MainComponent.h
    Source/UI/MainComponent.cpp
    Source/UI/LyricsView.h
    Source/UI/LyricsView.cpp
    Source/UI/MixerComponent.h
    Source/UI/MixerComponent.cpp
    Source/UI/SongListView.h
    Source/UI/SongListView.cpp
    Source/UI/YouTubePlayerComponent.h
    Source/UI/YouTubePlayerComponent.cpp
    Source/UI/QueueComponent.h
    Source/UI/SynthMixerWindow.h
    Source/UI/SettingsComponent.h
    Source/UI/SettingsComponent.cpp
    Source/UI/BottomBarComponent.h
    Source/UI/BottomBarComponent.cpp
    Source/UI/LookAndFeel_PK.h

    # Services
    Source/Services/YouTubeService.h
    Source/Services/YouTubeService.cpp

    # Core/Instruments
    Source/Core/Instruments/IInstrumentSource.h
    Source/Core/Instruments/SF2Source.h
    Source/Core/Instruments/SF2Source.cpp

    # Core/Routing (NEW location for MixerController)
    Source/Core/Routing/MixerController.h
    Source/Core/Routing/MixerController.cpp

    # Core/Plugins
    Source/Core/Plugins/PluginHost.h
    Source/Core/Plugins/PluginHost.cpp

    # Core/MIDI
    Source/Core/NcnParser.h
    Source/Core/NcnParser.cpp
    Source/Core/MidiPlayer.h
    Source/Core/MidiPlayer.cpp
    Source/Core/MidiHelper.h
    Source/Core/MidiHelper.cpp
    Source/Core/QueueManager.h
    Source/Core/QueueManager.cpp

    # Engine
    Source/Core/KaraokeEngine.h
    Source/Core/KaraokeEngine.cpp

    # Audio (SoundFontSynth.h kept as header only; TSF_IMPLEMENTATION is in SF2Source.cpp)
    Source/Audio/AudioGraphManager.h
    Source/Audio/AudioGraphManager.cpp
    Source/Audio/SoundFontSynth.h

    # Database
    Source/Database/sqlite3.c
    Source/Database/DatabaseManager.h
    Source/Database/DatabaseManager.cpp
    Source/Database/LibraryScanner.h
    Source/Database/LibraryScanner.cpp
)

# Add Source directory to include path
target_include_directories(PKKaraokeVST3 PRIVATE Source)

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
