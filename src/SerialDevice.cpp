// © Kay Sievers <kay@vrfy.org>, 2020-2021
// SPDX-License-Identifier: Apache-2.0

#include "V2MIDI.h"

void V2MIDI::SerialDevice::begin() {
  _uart->begin(31250);
  _uart->setTimeout(1);
}

bool V2MIDI::SerialDevice::send(Packet *midi) {
  switch (midi->getType()) {
    case Packet::Status::NoteOn:
    case Packet::Status::NoteOff:
    case Packet::Status::ControlChange:
    case Packet::Status::PitchBend:
    case Packet::Status::SystemSongPosition:
      return _uart->write(midi->_data + 1, 3);
      return true;

    case Packet::Status::ProgramChange:
    case Packet::Status::AftertouchChannel:
    case Packet::Status::SystemTimeCodeQuarterFrame:
    case Packet::Status::SystemSongSelect:
      return _uart->write(midi->_data + 1, 2);
      return true;

    case Packet::Status::SystemTuneRequest:
    case Packet::Status::SystemClock:
    case Packet::Status::SystemStart:
    case Packet::Status::SystemContinue:
    case Packet::Status::SystemStop:
    case Packet::Status::SystemActiveSensing:
    case Packet::Status::SystemReset:
      return _uart->write(midi->_data[1]);
      return true;

    // For now, System Exclusive is only handled by the serial adapter itself,
    // not forwarded from or to the connected serial device.
    case Packet::Status::SystemExclusive:
      return false;
  }

  return false;
}

bool V2MIDI::SerialDevice::receive(Packet *midi) {
  if (_uart->available() == 0)
    return false;

  uint8_t b = _uart->read();
  if (b & 0x80)
    _state = State::Status;

  switch (_state) {
    case State::Idle:
      return false;

    case State::Status: {
      _channel = b & 0x0f;
      _status  = (Packet::Status)(b & 0xf0);
      switch (_status) {
        // Single byte message.
        case Packet::Status::SystemTuneRequest:
        case Packet::Status::SystemClock:
        case Packet::Status::SystemStart:
        case Packet::Status::SystemContinue:
        case Packet::Status::SystemStop:
        case Packet::Status::SystemActiveSensing:
        case Packet::Status::SystemReset:
          midi->set(_channel, _status, 0, 0);
          _state = State::Idle;
          return true;

        // Wait for next byte.
        case Packet::Status::ProgramChange:
        case Packet::Status::AftertouchChannel:
        case Packet::Status::SystemTimeCodeQuarterFrame:
        case Packet::Status::SystemSongSelect:
        case Packet::Status::NoteOn:
        case Packet::Status::NoteOff:
        case Packet::Status::ControlChange:
        case Packet::Status::PitchBend:
        case Packet::Status::SystemSongPosition:
          _state = State::Data1;
          return false;

        case Packet::Status::SystemExclusive:
          _state = State::SysEx;
          return false;
      }

    } break;

    case State::Data1:
      switch (_status) {
        // Two bytes message.
        case Packet::Status::ProgramChange:
        case Packet::Status::AftertouchChannel:
        case Packet::Status::SystemTimeCodeQuarterFrame:
        case Packet::Status::SystemSongSelect:
          midi->set(_channel, _status, b, 0);
          _state = State::Idle;
          return true;

        // Wait for next byte.
        case Packet::Status::NoteOn:
        case Packet::Status::NoteOff:
        case Packet::Status::ControlChange:
        case Packet::Status::PitchBend:
        case Packet::Status::SystemSongPosition:
          _data1 = b;
          _state = State::Data2;
          return false;
      }
      break;

    case State::Data2:
      midi->set(_channel, _status, _data1, b);
      _state = State::Idle;
      return true;

    case State::SysEx:
      // For now, System Exclusive is only handled by the serial adapter itself,
      // not forwarded from or to the connected serial device. Just discard
      // the bytes until the next status byte.
      return false;
  }

  return false;
}
