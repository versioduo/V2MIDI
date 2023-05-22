// © Kay Sievers <kay@versioduo.com>, 2020-2023
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Registered Parameter Numbers.
namespace V2MIDI::RPN {
  enum {
    PitchBendSensitivity  = 0, // Value: MSB == semitones, LSB == cents
    FineTuning            = 1, // Value: 100/8192 cents
    CoarseTuning          = 2, // Value: semitones
    TuningProgramSelect   = 3,
    TuningBankSelect      = 4,
    ModulationDepthRange  = 5,
    MPEConfiguration      = 6,
    ThreeDimensionalSound = (0x3d << 7),         // All LSB values are reserved.
    Null                  = ((0x7f << 7) | 0x7f) // De-select current NRPN, RPN.
  };
};
