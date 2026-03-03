#include "UI/VstScannerComponent.h"

// Use the same storage parameters as PluginHost so plugin list data is
// persisted in a single file consistent across the application.
static juce::PropertiesFile *initProps(juce::ApplicationProperties &props) {
  juce::PropertiesFile::Options opts;
  opts.applicationName = "PK_Karaoke";
  opts.filenameSuffix = ".xml";
  opts.folderName = "PK_Karaoke";
  opts.storageFormat = juce::PropertiesFile::storeAsXML;
  opts.millisecondsBeforeSaving = 1000;
  props.setStorageParameters(opts);
  return props.getUserSettings();
}

// Dead man's pedal — if the scanner child process crashes while scanning a
// particular plugin, JUCE will black-list it so the next scan skips it.
static juce::File getDeadMansPedalFile() {
  return juce::File::getSpecialLocation(
             juce::File::userApplicationDataDirectory)
      .getChildFile("PK_Karaoke")
      .getChildFile("plugin_scan_deadmans.txt");
}

VstScannerComponent::VstScannerComponent(PluginHost &host)
    : pluginHost(host),
      pluginListComp(host.getFormatManager(), host.getKnownPluginList(),
                     getDeadMansPedalFile(), initProps(appProperties)) {
  addAndMakeVisible(pluginListComp);
}

VstScannerComponent::~VstScannerComponent() {
  // หยุด background scanner thread ก่อนเสมอ เพื่อป้องกัน race condition
  // ที่จะทำให้ thread เข้าไปอ่านหรือเขียน KnownPluginList ขณะที่ UI กำลังถูกทำลาย
  pluginListComp.setVisible(false);
  // ถอด component ออกก่อนเพื่อหยุด timer/update pendingที่ค้างอยู่
  removeAllChildren();

  appProperties.saveIfNeeded();
  // บันทึก KnownPluginList ลงไฟล์ XML ของ PluginHost
  // เพื่อให้โปรแกรมครั้งถัดไปโหลดรายการได้ทันที (ไม่ต้องแสกนซ้ำ)
  pluginHost.savePluginList();
}

void VstScannerComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff1e2128));
}

void VstScannerComponent::resized() {
  pluginListComp.setBounds(getLocalBounds().reduced(8));
}
