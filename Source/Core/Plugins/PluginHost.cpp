#include "Core/Plugins/PluginHost.h"

PluginHost::PluginHost() {
  formatManager.addDefaultFormats();
  DBG("PluginHost: Initialized with " << formatManager.getNumFormats()
                                      << " formats");
}

void PluginHost::scanDefaultFolders() {
  // VST3 default locations เธเธ Windows
  juce::Array<juce::File> defaultDirs;
  defaultDirs.add(juce::File("C:/Program Files/Common Files/VST3"));
  defaultDirs.add(juce::File("C:/Program Files (x86)/Common Files/VST3"));
  defaultDirs.add(
      juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
          .getChildFile("VST3"));

  for (auto &dir : defaultDirs) {
    if (dir.isDirectory()) {
      DBG("PluginHost: Scanning " + dir.getFullPathName());
      doScan(dir);
    }
  }

  scanComplete = true;
  DBG("PluginHost: Scan complete. Found " << foundPlugins.size()
                                          << " plugins.");

  if (onScanComplete)
    onScanComplete(foundPlugins.size());
}

void PluginHost::scanFolder(const juce::File &folder) {
  if (folder.isDirectory())
    doScan(folder);
}

void PluginHost::doScan(const juce::File &folder) {
  for (int i = 0; i < formatManager.getNumFormats(); ++i) {
    auto *format = formatManager.getFormat(i);

    juce::FileSearchPath searchPath;
    searchPath.add(folder);

    juce::OwnedArray<juce::PluginDescription> results;

    // เธชเนเธเธเธซเธฒ plugins เธ—เธธเธ subfolder เธ”เนเธงเธข
    const auto vst3Files = folder.findChildFiles(
        juce::File::findFilesAndDirectories, true, "*.vst3");

    for (const auto &file : vst3Files) {
      juce::OwnedArray<juce::PluginDescription> descs;
      format->findAllTypesForFile(descs, file.getFullPathName());

      for (auto *desc : descs) {
        auto *copy = new juce::PluginDescription(*desc);
        foundPlugins.add(copy);
        DBG("PluginHost: Found plugin: " + desc->name);

        if (onPluginFound)
          onPluginFound(*desc);
      }
    }
  }
}

std::unique_ptr<juce::AudioPluginInstance>
PluginHost::loadPlugin(const juce::PluginDescription &desc, double sampleRate,
                       int blockSize, juce::String &errorMessage) {
  return formatManager.createPluginInstance(desc, sampleRate, blockSize,
                                            errorMessage);
}

