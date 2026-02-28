#include "Core/MidiHelper.h"

std::vector<InstrumentGroup> MidiHelper::getDrumInstrumentTypes() {
  return {InstrumentGroup::Kick,       InstrumentGroup::Snare,
          InstrumentGroup::HandClap,   InstrumentGroup::HiTom,
          InstrumentGroup::MidTom,     InstrumentGroup::LowTom,
          InstrumentGroup::HiHat,      InstrumentGroup::CrashCymbal,
          InstrumentGroup::RideCymbal, InstrumentGroup::Tambourine,
          InstrumentGroup::Cowbell,    InstrumentGroup::Bongo,
          InstrumentGroup::Conga,      InstrumentGroup::Timbale,
          InstrumentGroup::ThaiChing,  InstrumentGroup::PercussionDrum};
}

InstrumentGroup MidiHelper::getInstrumentDrumType(int drumNote) {
  if (drumNote == 35 || drumNote == 36)
    return InstrumentGroup::Kick;
  if (drumNote == 38 || drumNote == 40)
    return InstrumentGroup::Snare;
  if (drumNote == 39)
    return InstrumentGroup::HandClap;
  if (drumNote == 48 || drumNote == 50)
    return InstrumentGroup::HiTom;
  if (drumNote == 45 || drumNote == 47)
    return InstrumentGroup::MidTom;
  if (drumNote == 41 || drumNote == 43)
    return InstrumentGroup::LowTom;
  if (drumNote == 42 || drumNote == 44 || drumNote == 46)
    return InstrumentGroup::HiHat;
  if (drumNote == 49 || drumNote == 52 || drumNote == 55 || drumNote == 57)
    return InstrumentGroup::CrashCymbal;
  if (drumNote == 51 || drumNote == 53 || drumNote == 59)
    return InstrumentGroup::RideCymbal;
  if (drumNote == 54)
    return InstrumentGroup::Tambourine;
  if (drumNote == 56)
    return InstrumentGroup::Cowbell;
  if (drumNote == 60 || drumNote == 61)
    return InstrumentGroup::Bongo;
  if (drumNote == 62 || drumNote == 63 || drumNote == 64)
    return InstrumentGroup::Conga;
  if (drumNote == 65 || drumNote == 66)
    return InstrumentGroup::Timbale;
  if (drumNote >= 80 && drumNote <= 83)
    return InstrumentGroup::ThaiChing;

  return InstrumentGroup::PercussionDrum;
}

InstrumentGroup MidiHelper::getInstrumentType(int instNumber) {
  if (instNumber < 0 || instNumber > 127)
    return InstrumentGroup::Piano;

  if (instNumber >= 0 && instNumber <= 7)
    return InstrumentGroup::Piano;
  if (instNumber == 8 || instNumber == 13)
    return InstrumentGroup::ThaiRanad;
  if (instNumber == 12)
    return InstrumentGroup::ThaiPongLang;
  if ((instNumber >= 9 && instNumber <= 11) || instNumber == 14 ||
      instNumber == 15)
    return InstrumentGroup::ChromaticPercussion;
  if (instNumber >= 16 && instNumber <= 20)
    return InstrumentGroup::Organ;
  if (instNumber == 21 || instNumber == 22)
    return InstrumentGroup::Accordion;
  if (instNumber == 23)
    return InstrumentGroup::ThaiKaen;
  if (instNumber == 24)
    return InstrumentGroup::AcousticGuitarNylon;
  if (instNumber == 25 || instNumber == 120)
    return InstrumentGroup::AcousticGuitarSteel;
  if (instNumber == 26 || instNumber == 28)
    return InstrumentGroup::ElectricGuitarJazz;
  if (instNumber == 27)
    return InstrumentGroup::ElectricGuitarClean;
  if (instNumber == 29 || instNumber == 31)
    return InstrumentGroup::OverdrivenGuitar;
  if (instNumber == 30)
    return InstrumentGroup::DistortionGuitar;
  if (instNumber >= 32 && instNumber <= 39)
    return InstrumentGroup::Bass;
  if (instNumber >= 40 && instNumber <= 47)
    return InstrumentGroup::Strings;
  if (instNumber >= 48 && instNumber <= 55)
    return InstrumentGroup::Ensemble;
  if (instNumber >= 56 && instNumber <= 63)
    return InstrumentGroup::Brass;
  if (instNumber >= 64 && instNumber <= 66)
    return InstrumentGroup::Saxophone;
  if (instNumber >= 67 && instNumber <= 71)
    return InstrumentGroup::Reed;
  if (instNumber == 73)
    return InstrumentGroup::ThaiKlui;
  if (instNumber == 75)
    return InstrumentGroup::ThaiWote;
  if ((instNumber >= 72 && instNumber <= 79) && instNumber != 73 &&
      instNumber != 75)
    return InstrumentGroup::Pipe;
  if (instNumber >= 80 && instNumber <= 87)
    return InstrumentGroup::SynthLead;
  if (instNumber >= 88 && instNumber <= 95)
    return InstrumentGroup::SynthPad;
  if (instNumber >= 96 && instNumber <= 103)
    return InstrumentGroup::SynthEffects;
  if (instNumber >= 104 && instNumber <= 110)
    return InstrumentGroup::Ethnic;
  if (instNumber == 117)
    return InstrumentGroup::ThaiPoengMang;

  return InstrumentGroup::Percussive;
}

juce::String MidiHelper::getThaiName(InstrumentGroup type) {
  switch (type) {
  case InstrumentGroup::Piano:
    return "Piano";
  case InstrumentGroup::ChromaticPercussion:
    return "Chromatic";
  case InstrumentGroup::ThaiRanad:
    return juce::String::fromUTF8("เธฃเธฐเธเธฒเธ”");
  case InstrumentGroup::ThaiPongLang:
    return juce::String::fromUTF8("เนเธเธเธฅเธฒเธ");
  case InstrumentGroup::Organ:
    return "Organ";
  case InstrumentGroup::Accordion:
    return "Accordion";
  case InstrumentGroup::ThaiKaen:
    return juce::String::fromUTF8("เนเธเธ");
  case InstrumentGroup::AcousticGuitarNylon:
    return "Gt.Nylon";
  case InstrumentGroup::AcousticGuitarSteel:
    return "Gt.Steel";
  case InstrumentGroup::ElectricGuitarJazz:
    return "Gt.Jazz";
  case InstrumentGroup::ElectricGuitarClean:
    return "Gt.Clean";
  case InstrumentGroup::OverdrivenGuitar:
    return "Gt.Over";
  case InstrumentGroup::DistortionGuitar:
    return "Gt.Dist";
  case InstrumentGroup::Bass:
    return "Bass";
  case InstrumentGroup::Strings:
    return "Strings";
  case InstrumentGroup::Ensemble:
    return "Ensemble";
  case InstrumentGroup::Brass:
    return "Brass";
  case InstrumentGroup::Saxophone:
    return "Sax";
  case InstrumentGroup::Reed:
    return "Reed";
  case InstrumentGroup::Pipe:
    return "Pipe";
  case InstrumentGroup::ThaiKlui:
    return juce::String::fromUTF8("เธเธฅเธธเนเธข");
  case InstrumentGroup::ThaiWote:
    return juce::String::fromUTF8("เนเธซเธงเธ”");
  case InstrumentGroup::SynthLead:
    return "SynthLead";
  case InstrumentGroup::SynthPad:
    return "SynthPad";
  case InstrumentGroup::SynthEffects:
    return "SynthEffects";
  case InstrumentGroup::Ethnic:
    return "Ethnic";
  case InstrumentGroup::ThaiPoengMang:
    return juce::String::fromUTF8("เน€เธเธดเธเธกเธฒเธ");
  case InstrumentGroup::Percussive:
    return "Percussive";

  case InstrumentGroup::Kick:
    return "Kick";
  case InstrumentGroup::Snare:
    return "Snare";
  case InstrumentGroup::HandClap:
    return "HandClap";
  case InstrumentGroup::HiTom:
    return "HiTom";
  case InstrumentGroup::MidTom:
    return "MidTom";
  case InstrumentGroup::LowTom:
    return "LowTom";
  case InstrumentGroup::HiHat:
    return "Hi-Hat";
  case InstrumentGroup::CrashCymbal:
    return "Crash";
  case InstrumentGroup::RideCymbal:
    return "Ride";
  case InstrumentGroup::Tambourine:
    return "Tambourine";
  case InstrumentGroup::Cowbell:
    return "Cowbell";
  case InstrumentGroup::Bongo:
    return "Bongo";
  case InstrumentGroup::Conga:
    return "Conga";
  case InstrumentGroup::Timbale:
    return "Timbale";
  case InstrumentGroup::ThaiChing:
    return juce::String::fromUTF8("เธเธดเนเธ");
  case InstrumentGroup::PercussionDrum:
    return "Percussion";

  case InstrumentGroup::VSTi1:
    return "VSTi 1";
  case InstrumentGroup::VSTi2:
    return "VSTi 2";
  case InstrumentGroup::VSTi3:
    return "VSTi 3";
  case InstrumentGroup::VSTi4:
    return "VSTi 4";
  case InstrumentGroup::VSTi5:
    return "VSTi 5";
  case InstrumentGroup::VSTi6:
    return "VSTi 6";
  case InstrumentGroup::VSTi7:
    return "VSTi 7";
  case InstrumentGroup::VSTi8:
    return "VSTi 8";

  case InstrumentGroup::ReverbBus:
    return "Reverb";
  case InstrumentGroup::DelayBus:
    return "Delay";
  case InstrumentGroup::ChorusBus:
    return "Chorus";
  case InstrumentGroup::MasterBus:
    return "Master";
  case InstrumentGroup::VocalBus:
    return "Vocals";

  default:
    return "Unknown";
  }
}

