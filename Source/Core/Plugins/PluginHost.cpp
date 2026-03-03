#include "Core/Plugins/PluginHost.h"

// ไฟล์ XML สำหรับบันทึก/โหลดรายการ Plugin (เหมือน Cubase)
static juce::File getPluginListFile() {
  return juce::File::getSpecialLocation(
             juce::File::userApplicationDataDirectory)
      .getChildFile("PK_Karaoke")
      .getChildFile("known_plugins.xml");
}

PluginHost::PluginHost() {
  formatManager.addFormat(new juce::VST3PluginFormat());
  DBG("PluginHost: Initialized with " << formatManager.getNumFormats()
                                      << " formats");

  // โหลดรายการ Plugin ที่บันทึกไว้จาก XML ทันทีเมื่อเปิดโปรแกรม
  // (ไม่ต้องแสกนใหม่ เหมือน Cubase)
  loadPluginList();
}

void PluginHost::loadPluginList() {
  auto f = getPluginListFile();
  if (!f.existsAsFile()) {
    DBG("PluginHost: No saved plugin list found at " + f.getFullPathName());
    return;
  }

  if (auto xml = juce::XmlDocument::parse(f)) {
    knownPluginList.recreateFromXml(*xml);
    DBG("PluginHost: Loaded " << knownPluginList.getNumTypes()
                              << " plugins from " << f.getFullPathName());
  }
}

void PluginHost::savePluginList() {
  auto f = getPluginListFile();
  f.getParentDirectory().createDirectory(); // สร้างโฟลเดอร์ถ้ายังไม่มี

  if (auto xml = knownPluginList.createXml()) {
    xml->writeTo(f);
    DBG("PluginHost: Saved " << knownPluginList.getNumTypes() << " plugins to "
                             << f.getFullPathName());
  }
}

void PluginHost::getPluginsFromKnownList(
    juce::Array<juce::PluginDescription> &out) const {
  out.clear();
  for (int i = 0; i < knownPluginList.getNumTypes(); ++i) {
    if (auto *desc = knownPluginList.getType(i))
      out.add(*desc);
  }
}

void PluginHost::scanDefaultFolders() {
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
  savePluginList(); // บันทึกทันทีหลังแสกน
  DBG("PluginHost: Scan complete. Found " << foundPlugins.size()
                                          << " plugins.");

  if (onScanComplete)
    onScanComplete(foundPlugins.size());
}

void PluginHost::scanFolder(const juce::File &folder) {
  if (folder.isDirectory()) {
    doScan(folder);
    savePluginList();
  }
}

void PluginHost::doScan(const juce::File &folder) {
  for (int i = 0; i < formatManager.getNumFormats(); ++i) {
    auto *format = formatManager.getFormat(i);

    const auto vst3Files = folder.findChildFiles(
        juce::File::findFilesAndDirectories, true, "*.vst3");

    for (const auto &file : vst3Files) {
      juce::OwnedArray<juce::PluginDescription> descs;
      format->findAllTypesForFile(descs, file.getFullPathName());

      for (auto *desc : descs) {
        knownPluginList.addType(*desc); // เพิ่มใน KnownPluginList ด้วย
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
