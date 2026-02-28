#pragma once
#include "Core/Routing/MixerController.h"
#include <JuceHeader.h>

/**
 * PluginHost โ€” เธชเนเธเธ VST3/VST2 folders เนเธฅเธฐ load plugins
 * เนเธเน JUCE AudioPluginFormatManager
 */
class PluginHost {
public:
  PluginHost();
  ~PluginHost() = default;

  /** เธชเนเธเธ VST3 plugin เธเธฒเธ default folders (Program Files/VST3, User/VST3 เธฏเธฅเธฏ)
   */
  void scanDefaultFolders();

  /** เธชเนเธเธ VST3 เธเธฒเธ folder เธ—เธตเนเธเธณเธซเธเธ”เน€เธญเธ */
  void scanFolder(const juce::File &folder);

  /** เธฃเธฒเธขเธเธฒเธฃ plugin เธ—เธตเนเธเธ */
  const juce::OwnedArray<juce::PluginDescription> &getFoundPlugins() const {
    return foundPlugins;
  }

  /** load plugin instance เธชเธณเธซเธฃเธฑเธเนเธเนเธเธฒเธ */
  std::unique_ptr<juce::AudioPluginInstance>
  loadPlugin(const juce::PluginDescription &desc, double sampleRate,
             int blockSize, juce::String &errorMessage);

  /** เน€เธเนเธเธงเนเธฒเธชเนเธเธเน€เธชเธฃเนเธเนเธฅเนเธงเธซเธฃเธทเธญเธขเธฑเธ */
  bool isScanComplete() const { return scanComplete; }

  /** callback เน€เธกเธทเนเธญเธชเนเธเธเน€เธเธญ plugin เนเธซเธกเน */
  std::function<void(const juce::PluginDescription &)> onPluginFound;

  /** callback เน€เธกเธทเนเธญเธชเนเธเธเน€เธชเธฃเนเธเธชเธดเนเธ */
  std::function<void(int totalFound)> onScanComplete;

private:
  juce::AudioPluginFormatManager formatManager;
  juce::OwnedArray<juce::PluginDescription> foundPlugins;
  juce::KnownPluginList knownPluginList;

  bool scanComplete = false;

  void doScan(const juce::File &folder);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginHost)
};

