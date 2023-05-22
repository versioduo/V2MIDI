// © Kay Sievers <kay@versioduo.com>, 2020-2023
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Packet.h"
#include "Transport.h"
#include <V2Base.h>

namespace V2MIDI {
class SerialDevice : public Transport {
public:
  struct {
    uint32_t input{};
    uint32_t output{};
  } statistics;

  constexpr SerialDevice(Uart *uart) : _uart(uart) {}
  void begin() {
    _uart->begin(31250);
    _uart->setTimeout(1);
  }

  bool send(Packet *midi) {
    switch (midi->getType()) {
      case Packet::Status::NoteOn:
      case Packet::Status::NoteOff:
      case Packet::Status::ControlChange:
      case Packet::Status::PitchBend:
      case Packet::Status::SystemSongPosition:
        return _uart->write(midi->_data + 1, 3);
        statistics.output++;
        return true;

      case Packet::Status::ProgramChange:
      case Packet::Status::AftertouchChannel:
      case Packet::Status::SystemTimeCodeQuarterFrame:
      case Packet::Status::SystemSongSelect:
        return _uart->write(midi->_data + 1, 2);
        statistics.output++;
        return true;

      case Packet::Status::SystemTuneRequest:
      case Packet::Status::SystemClock:
      case Packet::Status::SystemStart:
      case Packet::Status::SystemContinue:
      case Packet::Status::SystemStop:
      case Packet::Status::SystemActiveSensing:
      case Packet::Status::SystemReset:
        return _uart->write(midi->_data[1]);
        statistics.output++;
        return true;

      // System Exclusive is only handled right now.
      case Packet::Status::SystemExclusive:
        return false;
    }

    return false;
  }

  bool receive(Packet *midi) {
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
            statistics.input++;
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
            statistics.input++;
            return true;

          // Wait for next byte.
          case Packet::Status::NoteOn:
          case Packet::Status::NoteOff:
          case Packet::Status::Aftertouch:
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
        statistics.input++;
        return true;

      case State::SysEx:
        // System Exclusive is not processed right now. Just discard
        // the bytes until the next status byte.
        return false;
    }

    return false;
  }

private:
  Uart *_uart;
  enum class State {
    Idle,
    Status,
    Data1,
    Data2,
    SysEx,
  } _state{};
  uint8_t _channel{};
  Packet::Status _status{};
  uint8_t _data1{};
};
};
