#pragma once

namespace V2MIDI {
  // The octave numbers -2 to 8 are not defined by MIDI itself, it's just what
  // some vendors of instruments and audio workstation software use. The middle
  // C (MIDI Note 60) in this mapping is C(3).
  static constexpr int8_t  octaveOfMiddleC = 3;
  static constexpr uint8_t C(int8_t octave) {
    return (octave + octaveOfMiddleC - 1) * 12;
  }

  static constexpr uint8_t Cs(int8_t octave) {
    return C(octave) + 1;
  }

  static constexpr uint8_t D(int8_t octave) {
    return C(octave) + 2;
  }

  static constexpr uint8_t Ds(int8_t octave) {
    return C(octave) + 3;
  }

  static constexpr uint8_t E(int8_t octave) {
    return C(octave) + 4;
  }

#undef F
  static constexpr uint8_t F(int8_t octave) {
    return C(octave) + 5;
  }

  static constexpr uint8_t Fs(int8_t octave) {
    return C(octave) + 6;
  }

  static constexpr uint8_t G(int8_t octave) {
    return C(octave) + 7;
  }

  static constexpr uint8_t Gs(int8_t octave) {
    return C(octave) + 8;
  }

  static constexpr uint8_t A(int8_t octave) {
    return C(octave) + 9;
  }

  static constexpr uint8_t As(int8_t octave) {
    return C(octave) + 10;
  }

  static constexpr uint8_t B(int8_t octave) {
    return C(octave) + 11;
  }
};
