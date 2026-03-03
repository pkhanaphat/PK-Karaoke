#pragma once

#include "Core/Plugins/PluginHost.h"
#include <JuceHeader.h>

class VstScannerComponent : public juce::Component {
public:
  VstScannerComponent(PluginHost &host);
  ~VstScannerComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  PluginHost &pluginHost;

  // Must be declared before pluginListComp so it's initialized first
  juce::ApplicationProperties appProperties;

  juce::PluginListComponent pluginListComp;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VstScannerComponent)
};
