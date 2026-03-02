#pragma once

#include "Core/KaraokeEngine.h"
#include <JuceHeader.h>
#include <functional>

class SplashComponent : public juce::Component, private juce::Timer {
public:
  SplashComponent(KaraokeEngine &engine,
                  std::function<void()> onFinishedCallback);
  ~SplashComponent() override;

  void paint(juce::Graphics &g) override;
  void resized() override;
  void timerCallback() override;

private:
  KaraokeEngine &karaokeEngine;
  std::function<void()> onFinished;

  juce::Label statusLabel;
  juce::ProgressBar progressBar;
  double progress = 0.0;

  juce::String currentStatus;
  int currentStep = 0;

  juce::String loadSetting(const juce::String &key);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplashComponent)
};
