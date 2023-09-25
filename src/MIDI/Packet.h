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
  uint8_t getPort() const {
    return _data[0] >> 4;
  }

  void setPort(uint8_t port) {
    _data[0] &= 0x0f;
    _data[0] |= port << 4;
  }

  uint8_t getChannel() const {
    return _data[1] & 0x0f;
  }

  void setChannel(uint8_t channel) {
    _data[1] &= 0xf0;
    _data[1] |= channel;
  }

  static Status getStatus(uint8_t b) {
    // Remove channel number.
    Status status = static_cast<Status>(b & 0xf0);
    if (status != Status::System)
      return status;

    // 'System' messages carry their message type.
    return static_cast<Status>(b);
  }

  Status getType() const {
    return getStatus(_data[1]);
  }

  uint8_t getNote() const {
    return _data[2];
  }

  uint8_t getNoteVelocity() const {
    return _data[3];
  }

  uint8_t getAftertouchNote() const {
    return _data[2];
  }

  uint8_t getAftertouch() const {
    return _data[3];
  }

  uint8_t getController() const {
    return _data[2];
  }

  uint8_t getControllerValue() const {
    return _data[3];
  }

  uint8_t getProgram() const {
    return _data[2];
  }

  uint8_t getAftertouchChannel() const {
    return _data[2];
  }

  int16_t getPitchBend() const {
    // 14 bit – 8192..8191.
    const int16_t value = _data[3] << 7 | _data[2];
    return value - 8192;
  }

  uint16_t getSongPosition() const {
    return _data[3] << 7 | _data[2];
  }

  uint16_t getSongSelect() const {
    return _data[2];
  }

  const uint8_t *getData() const {
    return _data;
  }

  Packet *setData(const uint8_t data[4]) {
    memcpy(_data, data, 4);
    return this;
  }

  // Encode values into the packet and return is own pointer to allow the
  // stacking of function calls.
  Packet *set(uint8_t channel, Status type, uint8_t data1 = 0, uint8_t data2 = 0) {
    _data[0] &= 0xf0;

    switch (type) {
      case Status::NoteOff:
        _data[0] |= static_cast<uint8_t>(CodeIndex::NoteOff);
        break;

      case Status::NoteOn:
        _data[0] |= static_cast<uint8_t>(CodeIndex::NoteOn);
        break;

      case Status::Aftertouch:
        _data[0] |= static_cast<uint8_t>(CodeIndex::Aftertouch);
        break;

      case Status::ControlChange:
        _data[0] |= static_cast<uint8_t>(CodeIndex::ControlChange);
        break;

      case Status::ProgramChange:
        _data[0] |= static_cast<uint8_t>(CodeIndex::ProgramChange);
        break;

      case Status::AftertouchChannel:
        _data[0] |= static_cast<uint8_t>(CodeIndex::AftertouchChannel);
        break;

      case Status::PitchBend:
        _data[0] |= static_cast<uint8_t>(CodeIndex::PitchBend);
        break;

      // System messages are global and encode their message type in
      // the 'channel number'.
      case Status::SystemSongSelect:
      case Status::SystemTimeCodeQuarterFrame:
        if (channel > 0)
          return NULL;
        _data[0] |= static_cast<uint8_t>(CodeIndex::SystemCommon2);
        break;

      case Status::SystemSongPosition:
        if (channel > 0)
          return NULL;
        _data[0] |= static_cast<uint8_t>(CodeIndex::SystemCommon3);
        break;

      case Status::SystemTuneRequest:
      case Status::SystemClock:
      case Status::SystemStart:
      case Status::SystemContinue:
      case Status::SystemStop:
      case Status::SystemActiveSensing:
      case Status::SystemReset:
        if (channel > 0)
          return NULL;

        _data[0] |= static_cast<uint8_t>(CodeIndex::SingleByte);
        break;

      // System Exclusive messages have their own API
      default:
        return NULL;
    }

    _data[1] = static_cast<uint8_t>(type) | channel;
    _data[2] = data1;
    _data[3] = data2;
    return this;
  }

  Packet *setNote(uint8_t channel, uint8_t note, uint8_t velocity) {
    // "64 appears to be a reasonable compromise for devices which respond to NoteOff velocity."
    if (velocity == 0)
      return set(channel, Status::NoteOff, note, 64);

    return set(channel, Status::NoteOn, note, velocity);
  }

  Packet *setNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
    return set(channel, Status::NoteOff, note, velocity);
  }

  Packet *setAftertouch(uint8_t channel, uint8_t note, uint8_t pressure) {
    return set(channel, Status::Aftertouch, note, pressure);
  }

  Packet *setControlChange(uint8_t channel, uint8_t controller, uint8_t value = 0) {
    return set(channel, Status::ControlChange, controller, value);
  }

  Packet *setAftertouchChannel(uint8_t channel, uint8_t pressure) {
    return set(channel, Status::AftertouchChannel, pressure, 0);
  }

  Packet *setProgram(uint8_t channel, uint8_t value) {
    return set(channel, Status::ProgramChange, value, 0);
  }

  Packet *setPitchBend(uint8_t channel, int16_t value) {
    // 14 bit – 8192..8191.
    const uint16_t bits = value + 8192;
    return set(channel, Status::PitchBend, bits & 0x7f, (bits >> 7) & 0x7f);
  }

private:
  friend class Port;
  friend class SerialDevice;
  friend class USBDevice;
  uint8_t _data[4]{};
};
};
