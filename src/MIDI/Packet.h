// © Kay Sievers <kay@versioduo.com>, 2020-2023
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <cstring>

namespace V2MIDI {
  class Packet {
  public:
    // USB MIDI packet - every packet is 4 bytes long.
    // 1. header (4 bits virtual port/wire number + 4 bits code index number)
    // 2. status (7th bit set + 3 bits type + 4 bits channel/system number)
    // 3. data byte 1 (7 bit)
    // 4. data byte 2 (7 bit)
    enum class CodeIndex : uint8_t {
      Reserved             = 0,
      Cable                = 1,
      SystemCommon2        = 2,
      SystemCommon3        = 3,
      SystemExclusiveStart = 4,
      SystemExclusiveEnd1  = 5,
      SystemExclusiveEnd2  = 6,
      SystemExclusiveEnd3  = 7,
      NoteOff              = 8,
      NoteOn               = 9,
      Aftertouch           = 10,
      ControlChange        = 11,
      ProgramChange        = 12,
      AftertouchChannel    = 13,
      PitchBend            = 14,
      SingleByte           = 15
    };

    // MIDI status byte (bit 4 to 7, bit 7 is always set).
    enum class Status : uint8_t {
      NoteOff           = 0x80 | (0 << 4), // [note, velocity]
      NoteOn            = 0x80 | (1 << 4), // [note, velocity]
      Aftertouch        = 0x80 | (2 << 4), // [note, pressure]
      ControlChange     = 0x80 | (3 << 4), // [control function, value]
      ProgramChange     = 0x80 | (4 << 4), // [program]
      AftertouchChannel = 0x80 | (5 << 4), // [pressure]
      PitchBend         = 0x80 | (6 << 4), // [value LSB, value MSB]
      System            = 0x80 | (7 << 4),

      // 'System' messages are device global, the channel number
      // indentifies the type of system message.
      SystemExclusive            = System | 0,  // [stream of 7-bit bytes terminated with 'ExclusiveEnd']
      SystemTimeCodeQuarterFrame = System | 1,  // [4 bits of timecode fragment]
      SystemSongPosition         = System | 2,  // [value LSB, value MSB]
      SystemSongSelect           = System | 3,  // [song]
      SystemTuneRequest          = System | 6,  // n/a
      SystemExclusiveEnd         = System | 7,  // n/a
      SystemClock                = System | 8,  // n/a
      SystemStart                = System | 10, // n/a
      SystemContinue             = System | 11, // n/a
      SystemStop                 = System | 12, // n/a
      SystemActiveSensing        = System | 14, // n/a
      SystemReset                = System | 15  // n/a
    };

    // Set virtual port/wire in the packet. Port 1 == 0.
    auto getPort() const -> uint8_t {
      return _data[0] >> 4;
    }

    auto setPort(uint8_t port) {
      _data[0] &= 0x0f;
      _data[0] |= port << 4;
    }

    auto getChannel() const -> uint8_t {
      return _data[1] & 0x0f;
    }

    auto setChannel(uint8_t channel) {
      _data[1] &= 0xf0;
      _data[1] |= channel;
    }

    static auto getStatus(uint8_t b) -> Status {
      // Remove channel number.
      if (auto status{Status(b & 0xf0)}; status != Status::System)
        return status;

      // 'System' messages carry their message type.
      return Status(b);
    }

    auto getType() const -> Status {
      return getStatus(_data[1]);
    }

    auto getNote() const -> uint8_t {
      return _data[2];
    }

    auto getNoteVelocity() const -> uint8_t {
      return _data[3];
    }

    auto getAftertouchNote() const -> uint8_t {
      return _data[2];
    }

    auto getAftertouch() const -> uint8_t {
      return _data[3];
    }

    auto getController() const -> uint8_t {
      return _data[2];
    }

    auto getControllerValue() const {
      return _data[3];
    }

    auto getProgram() const -> uint8_t {
      return _data[2];
    }

    auto getAftertouchChannel() const -> uint8_t {
      return _data[2];
    }

    auto getPitchBend() const -> uint16_t {
      // 14 bit – 8192..8191.
      auto value{int16_t(_data[3] << 7 | _data[2])};
      return value - 8192;
    }

    auto getSongPosition() const -> uint16_t {
      return _data[3] << 7 | _data[2];
    }

    auto getSongSelect() const -> uint16_t {
      return _data[2];
    }

    auto data() -> uint8_t* {
      return _data;
    }

    auto set(uint8_t data0, uint8_t data1 = 0, uint8_t data2 = 0) -> Packet* {
      switch (getStatus(data0)) {
        case Status::NoteOff:
          _data[0] |= uint8_t(CodeIndex::NoteOff);
          break;

        case Status::NoteOn:
          _data[0] |= uint8_t(CodeIndex::NoteOn);
          break;

        case Status::Aftertouch:
          _data[0] |= uint8_t(CodeIndex::Aftertouch);
          break;

        case Status::ControlChange:
          _data[0] |= uint8_t(CodeIndex::ControlChange);
          break;

        case Status::ProgramChange:
          _data[0] |= uint8_t(CodeIndex::ProgramChange);
          break;

        case Status::AftertouchChannel:
          _data[0] |= uint8_t(CodeIndex::AftertouchChannel);
          break;

        case Status::PitchBend:
          _data[0] |= uint8_t(CodeIndex::PitchBend);
          break;

        case Status::SystemSongSelect:
        case Status::SystemTimeCodeQuarterFrame:
          _data[0] |= uint8_t(CodeIndex::SystemCommon2);
          break;

        case Status::SystemSongPosition:
          _data[0] |= uint8_t(CodeIndex::SystemCommon3);
          break;

        case Status::SystemTuneRequest:
        case Status::SystemClock:
        case Status::SystemStart:
        case Status::SystemContinue:
        case Status::SystemStop:
        case Status::SystemActiveSensing:
        case Status::SystemReset:
          _data[0] |= uint8_t(CodeIndex::SingleByte);
          break;
      }

      _data[1] = data0;
      _data[2] = data1;
      _data[3] = data2;
      return this;
    }

    auto set(Status type, uint8_t channel, uint8_t data1 = 0, uint8_t data2 = 0) -> Packet* {
      _data[0] &= 0xf0;

      switch (type) {
        case Status::NoteOff:
          _data[0] |= uint8_t(CodeIndex::NoteOff);
          break;

        case Status::NoteOn:
          _data[0] |= uint8_t(CodeIndex::NoteOn);
          break;

        case Status::Aftertouch:
          _data[0] |= uint8_t(CodeIndex::Aftertouch);
          break;

        case Status::ControlChange:
          _data[0] |= uint8_t(CodeIndex::ControlChange);
          break;

        case Status::ProgramChange:
          _data[0] |= uint8_t(CodeIndex::ProgramChange);
          break;

        case Status::AftertouchChannel:
          _data[0] |= uint8_t(CodeIndex::AftertouchChannel);
          break;

        case Status::PitchBend:
          _data[0] |= uint8_t(CodeIndex::PitchBend);
          break;

        default:
          std::abort();
      }

      _data[1] = uint8_t(type) | channel;
      _data[2] = data1;
      _data[3] = data2;
      return this;
    }

    auto setSystem(Status type, uint8_t data1 = 0, uint8_t data2 = 0) -> Packet* {
      _data[0] &= 0xf0;

      switch (type) {
        case Status::SystemSongSelect:
        case Status::SystemTimeCodeQuarterFrame:
          _data[0] |= uint8_t(CodeIndex::SystemCommon2);
          break;

        case Status::SystemSongPosition:
          _data[0] |= uint8_t(CodeIndex::SystemCommon3);
          break;

        case Status::SystemTuneRequest:
        case Status::SystemClock:
        case Status::SystemStart:
        case Status::SystemContinue:
        case Status::SystemStop:
        case Status::SystemActiveSensing:
        case Status::SystemReset:
          _data[0] |= uint8_t(CodeIndex::SingleByte);
          break;

        default:
          std::abort();
      }

      _data[1] = uint8_t(type);
      _data[2] = data1;
      _data[3] = data2;
      return this;
    }

    auto setNote(uint8_t channel, uint8_t note, uint8_t velocity) -> Packet* {
      if (velocity == 0)
        return set(Status::NoteOff, channel, note, 64);

      return set(Status::NoteOn, channel, note, velocity);
    }

    auto setNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) -> Packet* {
      return set(Status::NoteOff, channel, note, velocity);
    }

    auto setAftertouch(uint8_t channel, uint8_t note, uint8_t pressure) -> Packet* {
      return set(Status::Aftertouch, channel, note, pressure);
    }

    auto setControlChange(uint8_t channel, uint8_t controller, uint8_t value = 0) -> Packet* {
      return set(Status::ControlChange, channel, controller, value);
    }

    auto setAftertouchChannel(uint8_t channel, uint8_t pressure) -> Packet* {
      return set(Status::AftertouchChannel, channel, pressure, 0);
    }

    auto setProgram(uint8_t channel, uint8_t value) {
      return set(Status::ProgramChange, channel, value, 0);
    }

    auto setPitchBend(uint8_t channel, int16_t value) {
      // 14 bit – 8192..8191.
      auto bits{uint16_t(value + 8192)};
      return set(Status::PitchBend, channel, bits & 0x7f, (bits >> 7) & 0x7f);
    }

  private:
    friend class Port;
    friend class SerialDevice;
    friend class USBDevice;
    uint8_t _data[4]{};
  };
};
