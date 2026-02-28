#pragma once
#include <JuceHeader.h>

/**
 * IInstrumentSource โ€” Interface เธชเธณเธซเธฃเธฑเธเธ—เธธเธ Instrument (SF2, VSTi เธฏเธฅเธฏ)
 * Track เนเธ•เนเธฅเธฐ track เธเธฐเธกเธต IInstrumentSource* เธซเธเธถเนเธเธ•เธฑเธง
 */
class IInstrumentSource {
public:
  virtual ~IInstrumentSource() = default;

  /** เน€เธ•เธฃเธตเธขเธก audio engine เธเนเธญเธเน€เธฅเนเธ */
  virtual void prepare(double sampleRate, int samplesPerBlock) = 0;

  /** เธเธทเธเธ—เธฃเธฑเธเธขเธฒเธเธฃเธ—เธฑเนเธเธซเธกเธ” */
  virtual void releaseResources() = 0;

  /** process MIDI โ’ audio */
  virtual void process(juce::AudioBuffer<float> &audioBuffer,
                       juce::MidiBuffer &midiBuffer) = 0;

  /** load instrument file (sf2, dll เธฏเธฅเธฏ) */
  virtual bool loadInstrument(const juce::File &file) = 0;

  /** เธเธทเนเธญ instrument เธชเธณเธซเธฃเธฑเธเนเธชเธ”เธเนเธ UI */
  virtual juce::String getName() const = 0;

  /** เธงเนเธฒ load เธชเธณเน€เธฃเนเธเธซเธฃเธทเธญเธขเธฑเธ */
  virtual bool isLoaded() const = 0;

  /** volume เนเธเธ linear (0.0 โ€“ 1.0) */
  virtual void setVolume(float linearGain) = 0;
  virtual float getVolume() const = 0;
};

