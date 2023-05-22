// © Kay Sievers <kay@versioduo.com>, 2020-2023
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace V2MIDI::GM {
  // MIDI Program Change numbers / instruments.
  namespace Program {
    enum {
      // Piano
      AcousticGrandPiano  = 0,
      BrightAcousticPiano = 1,
      ElectricGrandPiano  = 2,
      HonkyTonkPiano      = 3,
      ElectricPiano1      = 4,
      ElectricPiano2      = 5,
      Harpsichord         = 6,
      Clavi               = 7,

      // Chromatic Percussion
      Celesta      = 8,
      Glockenspiel = 9,
      MusicBox     = 10,
      Vibraphone   = 11,
      Marimba      = 12,
      Xylophone    = 13,
      TubularBells = 14,
      Dulcimer     = 15,

      // Organ
      DrawbarOrgan    = 16,
      PercussiveOrgan = 17,
      RockOrgan       = 18,
      ChurchOrgan     = 19,
      ReedOrgan       = 20,
      Accordion       = 21,
      Harmonica       = 22,
      TangoAccordion  = 23,

      // Guitar
      AcousticGuitarNylon  = 24,
      AcousticGuitarSsteel = 25,
      ElectricGuitarJazz   = 26,
      ElectricGuitarClean  = 27,
      ElectricGuitarMuted  = 28,
      OverdrivenGuitar     = 29,
      DistortionGuitar     = 30,
      GuitarHarmonics      = 31,

      // Bass
      AcousticBass       = 32,
      ElectricBassFinger = 33,
      ElectricBassPick   = 34,
      FretlessBass       = 35,
      SlapBass1          = 36,
      SlapBass2          = 37,
      SynthBass1         = 38,
      SynthBass2         = 39,

      // Strings
      Violin           = 40,
      Viola            = 41,
      Cello            = 42,
      Contrabass       = 43,
      TremoloStrings   = 44,
      PizzicatoStrings = 45,
      OrchestralHarp   = 46,

      // Ensemble
      Timpani         = 47,
      StringEnsemble1 = 48,
      StringEnsemble2 = 49,
      SynthStrings1   = 50,
      SynthStrings2   = 51,
      ChoirAahs       = 52,
      VoiceOohs       = 53,
      SynthVoice      = 54,
      OrchestraHit    = 55,

      // Brass
      Trumpet      = 56,
      Trombone     = 57,
      Tuba         = 58,
      MutedTrumpet = 59,
      FrenchHorn   = 60,
      BrassSection = 61,
      SynthBrass1  = 62,
      SynthBrass2  = 63,

      // Reed
      SopranoSax  = 64,
      AltoSax     = 65,
      TenorSax    = 66,
      BaritoneSax = 67,
      Oboe        = 68,
      EnglishHorn = 69,
      Bassoon     = 70,
      Clarinet    = 71,

      // Pipe
      Piccolo     = 72,
      Flute       = 73,
      Recorder    = 74,
      PanFlute    = 75,
      BlownBottle = 76,
      Shakuhachi  = 77,
      Whistle     = 78,
      Ocarina     = 79,

      // Synth Lead
      Lead1Square   = 80,
      Lead2Sawtooth = 81,
      Lead3Calliope = 82,
      Lead4Chiff    = 83,
      Lead5Charang  = 84,
      Lead6Voice    = 85,
      Lead7Ffifths  = 86,
      Lead8Bass     = 87,

      // Synth Pad
      Pad1NewAge    = 88,
      Pad2Warm      = 89,
      Pad3Polysynth = 90,
      Pad4Choir     = 91,
      Pad5Bowed     = 92,
      Pad6Metallic  = 93,
      Pad7Halo      = 94,
      Pad8Sweep     = 95,

      // Synth Effects
      FX1Rain       = 96,
      FX2Soundtrack = 97,
      FX3Crystal    = 98,
      FX4Atmosphere = 99,
      FX5Brightness = 100,
      FX6Goblins    = 101,
      FX7Echoes     = 102,
      FX8SciFi      = 103,

      // Ethnic Percussive
      Sitar    = 104,
      Banjo    = 105,
      Shamisen = 106,
      Koto     = 107,
      Kalimba  = 108,
      BagPipe  = 109,
      Fiddle   = 110,
      Shanai   = 111,

      // Percussive
      TinkleBell    = 112,
      Agogo         = 113,
      SteelDrums    = 114,
      Woodblock     = 115,
      TaikoDrum     = 116,
      MelodicTom    = 117,
      SynthDrum     = 118,
      ReverseCymbal = 119,

      // Sound Effects
      GuitarFretNoise = 120,
      BreathNoise     = 121,
      Seashore        = 122,
      BirdTweet       = 123,
      TelephoneRing   = 124,
      Helicopter      = 125,
      Applause        = 126,
      Gunshot         = 127
    };
  };

  // General MIDI percussion mapping. Traditionally on MIDI channel 10.
  namespace Percussion {
    enum {
      HighQ            = 27,
      Slap             = 28,
      ScratchPush      = 29,
      ScratchPull      = 30,
      Sticks           = 31,
      SquareClick      = 32,
      MetronomeClick   = 33,
      MetronomeBell    = 34,
      AcousticBassDrum = 35,
      BassDrum1        = 36,
      SideStick        = 37,
      AcousticSnare    = 38,
      HandClap         = 39,
      ElectricSnare    = 40,
      LowFloorTom      = 41,
      ClosedHiHat      = 42,
      HighFloorTom     = 43,
      PedalHiHat       = 44,
      LowTom           = 45,
      OpenHiHat        = 46,
      LowMidTom        = 47,
      HiMidTom         = 48,
      CrashCymbal1     = 49,
      HighTom          = 50,
      RideCymbal1      = 51,
      ChineseCymbal    = 52,
      RideBell         = 53,
      Tambourine       = 54,
      SplashCymbal     = 55,
      Cowbell          = 56,
      CrashCymbal2     = 57,
      Vibraslap        = 58,
      RideCymbal2      = 59,
      HiBongo          = 60,
      LowBongo         = 61,
      MuteHiConga      = 62,
      OpenHiConga      = 63,
      LowConga         = 64,
      HighTimbale      = 65,
      LowTimbale       = 66,
      HighAgogo        = 67,
      LowAgogo         = 68,
      Cabasa           = 69,
      Maracas          = 70,
      ShortWhistle     = 71,
      LongWhistle      = 72,
      ShortGuiro       = 73,
      LongGuiro        = 74,
      Claves           = 75,
      HiWoodBlock      = 76,
      LowWoodBlock     = 77,
      MuteCuica        = 78,
      OpenCuica        = 79,
      MuteTriangle     = 80,
      OpenTriangle     = 81,
      Shaker           = 82,
      JingleBell       = 83,
      BellTree         = 84,
      Castanets        = 85,
      MuteSurdo        = 86,
      OpenSurdo        = 87,
    };
  };
};
