// © Kay Sievers <kay@versioduo.com>, 2020-2023
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Clock.h"
#include "Packet.h"
#include "Transport.h"
#include <cstdlib>

namespace V2MIDI {
  // Transport-independent MIDI functional interface. Supports message parsing/dispatching,
  // system exclusive buffering/streaming, packet statistics.
  class Port {
  public:
    struct Counter {
      uint32_t packet;
      uint32_t note;
      uint32_t noteOff;
      uint32_t aftertouch;
      uint32_t control;
      uint32_t program;
      uint32_t aftertouchChannel;
      uint32_t pitchbend;
      struct {
        struct {
          uint32_t tick;
        } clock;
        uint32_t exclusive;
        uint32_t reset;
      } system;
    };

    constexpr Port(uint8_t index, uint32_t sysexSize) : _index{index}, _sysexSize{sysexSize} {}

    void begin() {
      // Buffer to store an incoming and outgoing SysEx messages. The buffer needs
      // to be able to carry a complete message. The message always starts with
      // 0xf0 (SystemExclusive) and ends with 0xf7 (SystemExclusiveEnd), all other
      // bytes carry 7-bit only.
      //
      // If no buffer is provided, incoming SysEx messages are discarded.
      _sysex.in.buffer  = (uint8_t*)malloc(_sysexSize);
      _sysex.out.buffer = (uint8_t*)malloc(_sysexSize);
    }

    // During dispatch(), replies can be sent back to the given 'transport'.
    void dispatch(Transport* transport, Packet* packet) {
      _statistics.input.packet++;

      if (!storeSystemExclusive(packet))
        return;

      if (packet->getType() != Packet::Status::SystemExclusive)
        handlePacket(packet);

      switch (packet->getType()) {
        case Packet::Status::NoteOn:
          _statistics.input.note++;
          handleNote(packet->getChannel(), packet->getNote(), packet->getNoteVelocity());
          break;

        case Packet::Status::NoteOff:
          _statistics.input.noteOff++;
          handleNoteOff(packet->getChannel(), packet->getNote(), packet->getNoteVelocity());
          break;

        case Packet::Status::Aftertouch:
          _statistics.input.aftertouch++;
          handleAftertouch(packet->getChannel(), packet->getAftertouchNote(), packet->getAftertouch());
          break;

        case Packet::Status::ControlChange:
          _statistics.input.control++;
          handleControlChange(packet->getChannel(), packet->getController(), packet->getControllerValue());
          break;

        case Packet::Status::ProgramChange:
          _statistics.input.program++;
          handleProgramChange(packet->getChannel(), packet->getProgram());
          break;

        case Packet::Status::AftertouchChannel:
          _statistics.input.aftertouchChannel++;
          handleAftertouchChannel(packet->getChannel(), packet->getAftertouchChannel());
          break;

        case Packet::Status::PitchBend:
          _statistics.input.pitchbend++;
          handlePitchBend(packet->getChannel(), packet->getPitchBend());
          break;

        case Packet::Status::SystemSongPosition:
          handleSongPosition(packet->getSongPosition());
          break;

        case Packet::Status::SystemSongSelect:
          handleSongSelect(packet->getSongSelect());
          break;

        case Packet::Status::SystemClock:
          _statistics.input.system.clock.tick++;
          handleClock(Clock::Event::Tick);
          break;

        case Packet::Status::SystemStart:
          handleClock(Clock::Event::Start);
          break;

        case Packet::Status::SystemContinue:
          handleClock(Clock::Event::Continue);
          break;

        case Packet::Status::SystemStop:
          handleClock(Clock::Event::Stop);
          break;

        case Packet::Status::SystemExclusive: {
          _statistics.input.system.exclusive++;
          handleSystemExclusive(transport, _sysex.in.buffer, _sysex.in.length);
          handleSystemExclusive(_sysex.in.buffer, _sysex.in.length);
        } break;

        case Packet::Status::SystemReset:
          _statistics.input.system.reset++;
          handleSystemReset();
          break;
      }
    }

    // Set the port's number in the outgoing packet and updates the statistics.
    bool send(Packet* packet) {
      // Do not interrupt a system exclusive transfer.
      if (_sysex.out.length > 0)
        return false;

      packet->setPort(_index);
      if (!handleSend(packet))
        return false;

      _statistics.output.packet++;

      switch (packet->getType()) {
        case Packet::Status::NoteOn:
          _statistics.output.note++;
          break;

        case Packet::Status::NoteOff:
          _statistics.output.noteOff++;
          break;

        case Packet::Status::Aftertouch:
          _statistics.output.aftertouch++;
          break;

        case Packet::Status::ControlChange:
          _statistics.output.control++;
          break;

        case Packet::Status::ProgramChange:
          _statistics.output.program++;
          break;

        case Packet::Status::AftertouchChannel:
          _statistics.output.aftertouchChannel++;
          break;

        case Packet::Status::PitchBend:
          _statistics.output.pitchbend++;
          break;

        case Packet::Status::SystemClock:
          _statistics.output.system.clock.tick++;
          break;

        case Packet::Status::SystemReset:
          _statistics.output.system.reset++;
          break;
      }

      return true;
    }

    // Get the raw buffer to copy the SysEx message into.
    uint8_t* getSystemExclusiveBuffer() {
      return _sysex.out.buffer;
    }

    // Prepare SysEx message to chunk into packets. Send as many packets as possible,
    // the remaining packets will be sent with loopSystemExclusive().
    void sendSystemExclusive(Transport* transport, uint32_t length) {
      if (length < 2)
        return;

      if (_sysex.out.buffer[0] != static_cast<uint8_t>(Packet::Status::SystemExclusive))
        return;

      if (_sysex.out.buffer[length - 1] != static_cast<uint8_t>(Packet::Status::SystemExclusiveEnd))
        return;

      _sysex.out.transport = transport;
      _sysex.out.length    = length;
      _sysex.out.position  = 0;

      // Send as many packets as possible.
      while (loopSystemExclusive() > 0)
        ;
    }

    void resetSystemExclusive() {
      _sysex.in.reset();
      _sysex.out.reset();
    }

    // Send the next packet over the specified transport. Returns:
    //  0: nothing to do,
    // -1: sending failed,
    //  1: there are remaining packets.
    int8_t loopSystemExclusive() {
      if (_sysex.out.length == 0)
        return 0;

      Packet         _packet;
      const uint32_t remain = _sysex.out.length - _sysex.out.position;
      switch (remain) {
        case 1:
          _packet._data[0] = (_index << 4) | static_cast<uint8_t>(Packet::CodeIndex::SystemExclusiveEnd1);
          _packet._data[1] = _sysex.out.buffer[_sysex.out.position];
          _packet._data[2] = 0;
          _packet._data[3] = 0;
          break;

        case 2:
          _packet._data[0] = (_index << 4) | static_cast<uint8_t>(Packet::CodeIndex::SystemExclusiveEnd2);
          _packet._data[1] = _sysex.out.buffer[_sysex.out.position];
          _packet._data[2] = _sysex.out.buffer[_sysex.out.position + 1];
          _packet._data[3] = 0;
          break;

        case 3:
          _packet._data[0] = (_index << 4) | static_cast<uint8_t>(Packet::CodeIndex::SystemExclusiveEnd3);
          _packet._data[1] = _sysex.out.buffer[_sysex.out.position];
          _packet._data[2] = _sysex.out.buffer[_sysex.out.position + 1];
          _packet._data[3] = _sysex.out.buffer[_sysex.out.position + 2];
          break;

        default:
          _packet._data[0] = (_index << 4) | static_cast<uint8_t>(Packet::CodeIndex::SystemExclusiveStart);
          _packet._data[1] = _sysex.out.buffer[_sysex.out.position];
          _packet._data[2] = _sysex.out.buffer[_sysex.out.position + 1];
          _packet._data[3] = _sysex.out.buffer[_sysex.out.position + 2];
          break;
      }

      if (!_sysex.out.transport) {
        if (!handleSend(&_packet))
          return -1;

      } else {
        if (!_sysex.out.transport->send(&_packet))
          return -1;
      }

      _statistics.output.packet++;

      if (remain > 3) {
        _sysex.out.position += 3;
        return 1;
      }

      _sysex.out.transport = NULL;
      _sysex.out.length    = 0;
      _statistics.output.system.exclusive++;
      return 0;
    }

  protected:
    const uint8_t  _index;
    const uint32_t _sysexSize;

    friend class Packet;
    struct {
      Counter input;
      Counter output;
    } _statistics{};

    virtual void handleNote(uint8_t channel, uint8_t note, uint8_t velocity) {}
    virtual void handleNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {}
    virtual void handleAftertouch(uint8_t channel, uint8_t note, uint8_t pressure) {}
    virtual void handleControlChange(uint8_t channel, uint8_t controller, uint8_t value) {}
    virtual void handleProgramChange(uint8_t channel, uint8_t value) {}
    virtual void handleAftertouchChannel(uint8_t channel, uint8_t pressure) {}
    virtual void handlePitchBend(uint8_t channel, int16_t value) {}
    virtual void handleSongPosition(uint16_t beats){};
    virtual void handleSongSelect(uint8_t number){};
    virtual void handleClock(Clock::Event clock) {}
    virtual void handleSystemExclusive(const uint8_t* buffer, uint32_t len) {}
    virtual void handleSystemReset() {}
    virtual void handleSwitchChannel(uint8_t channel) {}

    // All messages besides system exclusive.
    virtual void handlePacket(Packet* packet) {}

    // During dispatch, replies are sent back to the originating transport.
    virtual void handleSystemExclusive(Transport* transport, const uint8_t* buffer, uint32_t len) {}

    virtual bool handleSend(Packet* packet) {
      return false;
    }

  private:
    struct {
      struct {
        uint8_t* buffer;
        uint32_t length;
        bool     appending;

        void reset() {
          length    = 0;
          appending = false;
        }
      } in;

      struct {
        Transport* transport;
        uint8_t*   buffer;
        uint32_t   length;
        uint32_t   position;

        void reset() {
          length   = 0;
          position = 0;
        }
      } out;
    } _sysex{};

    bool storeSystemExclusive(Packet* packet) {
      switch (static_cast<Packet::CodeIndex>(packet->_data[0] & 0x0f)) {
        case Packet::CodeIndex::SystemCommon2:
        case Packet::CodeIndex::SystemCommon3:
        case Packet::CodeIndex::NoteOff:
        case Packet::CodeIndex::NoteOn:
        case Packet::CodeIndex::Aftertouch:
        case Packet::CodeIndex::ControlChange:
        case Packet::CodeIndex::ProgramChange:
        case Packet::CodeIndex::AftertouchChannel:
        case Packet::CodeIndex::PitchBend:
          // Return single packet message, discard any possible SysEx stream.
          _sysex.in.appending = false;
          _sysex.in.length    = 0;
          return true;

        case Packet::CodeIndex::SingleByte:
          // Single byte, like a system message.
          if (!_sysex.in.appending) {
            _sysex.in.reset();
            return true;
          }

          // Used in the middle of a SysEx packet stream to transport a single byte instead of three.
          if (_sysex.in.length + 1 > _sysexSize) {
            _sysex.in.reset();
            return false;
          }

          _sysex.in.buffer[_sysex.in.length++] = packet->_data[1];
          return false;

        // Start of a new SysEx stream, or append data to the current stream.
        case Packet::CodeIndex::SystemExclusiveStart:
          // Not enough space to store the stream.
          if (_sysex.in.length + 3 > _sysexSize) {
            _sysex.in.reset();
            return false;
          }

          if (!_sysex.in.appending) {
            _sysex.in.length = 0;

            // Must be the start of a SysEx.
            if (packet->_data[1] != static_cast<uint8_t>(Packet::Status::SystemExclusive))
              return false;

            _sysex.in.appending = true;
          }

          _sysex.in.buffer[_sysex.in.length++] = packet->_data[1];
          _sysex.in.buffer[_sysex.in.length++] = packet->_data[2];
          _sysex.in.buffer[_sysex.in.length++] = packet->_data[3];
          return false;

        // End of SysEx stream with various lengths.
        case Packet::CodeIndex::SystemExclusiveEnd1:
          // Invalid 'End' packet
          if (packet->_data[1] != static_cast<uint8_t>(Packet::Status::SystemExclusiveEnd)) {
            _sysex.in.reset();
            return false;
          }

          // 'End' packet without previous data, discarding.
          if (!_sysex.in.appending) {
            _sysex.in.length = 0;
            return false;
          }

          // Not enough space to store the stream.
          if (_sysex.in.length + 1 > _sysexSize) {
            _sysex.in.reset();
            return false;
          }

          _sysex.in.buffer[_sysex.in.length++] = packet->_data[1];
          break;

        case Packet::CodeIndex::SystemExclusiveEnd2:
          // Invalid 'End' packet.
          if (packet->_data[2] != static_cast<uint8_t>(Packet::Status::SystemExclusiveEnd)) {
            _sysex.in.reset();
            return false;
          }

          // Not enough space to store the stream.
          if (_sysex.in.length + 2 > _sysexSize) {
            _sysex.in.reset();
            return false;
          }

          // Single 'End' packet.
          if (!_sysex.in.appending) {
            _sysex.in.length = 0;

            // Must be an 'empty' SysEx.
            if (packet->_data[1] != static_cast<uint8_t>(Packet::Status::SystemExclusive))
              return false;
          }

          _sysex.in.buffer[_sysex.in.length++] = packet->_data[1];
          _sysex.in.buffer[_sysex.in.length++] = packet->_data[2];
          break;

        case Packet::CodeIndex::SystemExclusiveEnd3:
          // Invalid 'End' packet.
          if (packet->_data[3] != static_cast<uint8_t>(Packet::Status::SystemExclusiveEnd)) {
            _sysex.in.reset();
            return false;
          }

          // Not enough space to store the stream.
          if (_sysex.in.length + 3 > _sysexSize) {
            _sysex.in.reset();
            return false;
          }

          // Single 'End' packet.
          if (!_sysex.in.appending) {
            _sysex.in.length = 0;

            // Must be a 'one byte' SysEx.
            if (packet->_data[1] != static_cast<uint8_t>(Packet::Status::SystemExclusive))
              return false;
          }

          _sysex.in.buffer[_sysex.in.length++] = packet->_data[1];
          _sysex.in.buffer[_sysex.in.length++] = packet->_data[2];
          _sysex.in.buffer[_sysex.in.length++] = packet->_data[3];
          break;

        default:
          _sysex.in.reset();
          return false;
      }

      // Always return 'SystemExclusive' as type.
      _sysex.in.appending = false;
      packet->_data[1]    = static_cast<uint8_t>(Packet::Status::SystemExclusive);
      return true;
    }
  };
};
