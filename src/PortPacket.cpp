// © Kay Sievers <kay@vrfy.org>, 2020-2021
// SPDX-License-Identifier: Apache-2.0

#include "V2MIDI.h"

// USB MIDI packet - every packet is 4 bytes long
// 1. header (4 bits virtual port/wire number + 4 bits code index number)
// 2. status (7th bit set + 3 bits type + 4 bits channel/system number)
// 3. data byte 1 (7 bit)
// 4. data byte 2 (7 bit)
enum class CodeIndex {
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

void V2MIDI::Port::begin() {
  // Buffer to store an incoming and outgoing SysEx messages. The buffer needs
  // to be able to carry a complete message. The message always starts with
  // 0xf0 (SystemExclusive) and ends with 0xf7 (SystemExclusiveEnd), all other
  // bytes carry 7-bit only.
  //
  // If no buffer is provided, incoming SysEx messages are discarded.
  _sysex.in.buffer  = (uint8_t *)malloc(_sysex.size);
  _sysex.out.buffer = (uint8_t *)malloc(_sysex.size);
}

void V2MIDI::Port::dispatch(V2MIDI::Transport *transport, V2MIDI::Packet *packet) {
  _statistics.input.packet++;

  if (!storeSystemExclusive(packet))
    return;

  if (packet->getType() != V2MIDI::Packet::Status::SystemExclusive)
    handlePacket(packet);

  switch (packet->getType()) {
    case V2MIDI::Packet::Status::NoteOn:
      _statistics.input.note++;
      handleNote(packet->getChannel(), packet->getNote(), packet->getNoteVelocity());
      break;

    case V2MIDI::Packet::Status::NoteOff:
      _statistics.input.note_off++;
      handleNoteOff(packet->getChannel(), packet->getNote(), packet->getNoteVelocity());
      break;

    case V2MIDI::Packet::Status::Aftertouch:
      _statistics.input.aftertouch++;
      handleAftertouch(packet->getChannel(), packet->getAftertouchNote(), packet->getAftertouch());
      break;

    case V2MIDI::Packet::Status::ControlChange:
      _statistics.input.control++;
      handleControlChange(packet->getChannel(), packet->getController(), packet->getControllerValue());
      break;

    case V2MIDI::Packet::Status::ProgramChange:
      _statistics.input.program++;
      handleProgramChange(packet->getChannel(), packet->getProgram());
      break;

    case V2MIDI::Packet::Status::AftertouchChannel:
      _statistics.input.aftertouch_channel++;
      handleAftertouchChannel(packet->getChannel(), packet->getAftertouchChannel());
      break;

    case V2MIDI::Packet::Status::PitchBend:
      _statistics.input.pitchbend++;
      handlePitchBend(packet->getChannel(), packet->getPitchBend());
      break;

    case V2MIDI::Packet::Status::SystemSongPosition:
      handleSongPosition(packet->getSongPosition());
      break;

    case V2MIDI::Packet::Status::SystemSongSelect:
      handleSongSelect(packet->getSongSelect());
      break;

    case V2MIDI::Packet::Status::SystemClock:
      _statistics.input.system.clock.tick++;
      handleClock(Packet::Clock::Tick);
      break;

    case V2MIDI::Packet::Status::SystemStart:
      handleClock(Packet::Clock::Start);
      break;

    case V2MIDI::Packet::Status::SystemContinue:
      handleClock(Packet::Clock::Continue);
      break;

    case V2MIDI::Packet::Status::SystemStop:
      handleClock(Packet::Clock::Stop);
      break;

    case V2MIDI::Packet::Status::SystemExclusive: {
      _statistics.input.system.exclusive++;
      handleSystemExclusive(transport, _sysex.in.buffer, _sysex.in.length);
      handleSystemExclusive(_sysex.in.buffer, _sysex.in.length);
    } break;

    case V2MIDI::Packet::Status::SystemReset:
      _statistics.input.system.reset++;
      handleSystemReset();
      break;
  }
}

bool V2MIDI::Port::send(V2MIDI::Packet *packet) {
  // Do not interrupt a system exclusive transfer.
  if (_sysex.out.length > 0)
    return false;

  packet->setPort(_index);
  if (!handleSend(packet))
    return false;

  _statistics.output.packet++;

  switch (packet->getType()) {
    case V2MIDI::Packet::Status::NoteOn:
      _statistics.output.note++;
      break;

    case V2MIDI::Packet::Status::NoteOff:
      _statistics.output.note_off++;
      break;

    case V2MIDI::Packet::Status::Aftertouch:
      _statistics.output.aftertouch++;
      break;

    case V2MIDI::Packet::Status::ControlChange:
      _statistics.output.control++;
      break;

    case V2MIDI::Packet::Status::ProgramChange:
      _statistics.output.program++;
      break;

    case V2MIDI::Packet::Status::AftertouchChannel:
      _statistics.output.aftertouch_channel++;
      break;

    case V2MIDI::Packet::Status::PitchBend:
      _statistics.output.pitchbend++;
      break;

    case V2MIDI::Packet::Status::SystemClock:
      _statistics.output.system.clock.tick++;
      break;

    case V2MIDI::Packet::Status::SystemReset:
      _statistics.output.system.reset++;
      break;
  }

  return true;
}

bool V2MIDI::Port::storeSystemExclusive(V2MIDI::Packet *packet) {
  switch (static_cast<CodeIndex>(packet->_data[0] & 0x0f)) {
    case CodeIndex::SystemCommon2:
    case CodeIndex::SystemCommon3:
    case CodeIndex::NoteOff:
    case CodeIndex::NoteOn:
    case CodeIndex::Aftertouch:
    case CodeIndex::ControlChange:
    case CodeIndex::ProgramChange:
    case CodeIndex::AftertouchChannel:
    case CodeIndex::PitchBend:
      // Return single packet message, discard any possible current SysEx stream
      _sysex.in.appending = false;
      _sysex.in.length    = 0;
      return true;

    case CodeIndex::SingleByte:
      // Single byte, like a system message.
      if (!_sysex.in.appending) {
        _sysex.in.appending = false;
        _sysex.in.length    = 0;
        return true;
      }

      // Used in the middle of a SysEx packet stream to transport a single byte instead of three.
      if (_sysex.in.length + 1 > _sysex.size) {
        _sysex.in.appending = false;
        _sysex.in.length    = 0;
        return false;
      }

      _sysex.in.buffer[_sysex.in.length++] = packet->_data[1];
      return false;

    // Start of a new SysEx stream, or append data to the current stream
    case CodeIndex::SystemExclusiveStart:
      // Not enough space to store the stream
      if (_sysex.in.length + 3 > _sysex.size) {
        _sysex.in.appending = false;
        _sysex.in.length    = 0;
        return false;
      }

      if (!_sysex.in.appending) {
        _sysex.in.length = 0;

        // Must be the start of a SysEx
        if (packet->_data[1] != static_cast<uint8_t>(Packet::Status::SystemExclusive))
          return false;

        _sysex.in.appending = true;
      }

      _sysex.in.buffer[_sysex.in.length++] = packet->_data[1];
      _sysex.in.buffer[_sysex.in.length++] = packet->_data[2];
      _sysex.in.buffer[_sysex.in.length++] = packet->_data[3];
      return false;

    // End of SysEx stream with various lengths
    case CodeIndex::SystemExclusiveEnd1:
      // Invalid 'End' packet
      if (packet->_data[1] != static_cast<uint8_t>(Packet::Status::SystemExclusiveEnd)) {
        _sysex.in.appending = false;
        _sysex.in.length    = 0;
        return false;
      }

      // 'End' packet without previous data, discarding
      if (!_sysex.in.appending) {
        _sysex.in.length = 0;
        return false;
      }

      // Not enough space to store the stream
      if (_sysex.in.length + 1 > _sysex.size) {
        _sysex.in.appending = false;
        _sysex.in.length    = 0;
        return false;
      }

      _sysex.in.buffer[_sysex.in.length++] = packet->_data[1];
      break;

    case CodeIndex::SystemExclusiveEnd2:
      // Invalid 'End' packet
      if (packet->_data[2] != static_cast<uint8_t>(Packet::Status::SystemExclusiveEnd)) {
        _sysex.in.appending = false;
        _sysex.in.length    = 0;
        return false;
      }

      // Not enough space to store the stream
      if (_sysex.in.length + 2 > _sysex.size) {
        _sysex.in.appending = false;
        _sysex.in.length    = 0;
        return false;
      }

      // Single 'End' packet
      if (!_sysex.in.appending) {
        _sysex.in.length = 0;

        // Must be an 'empty' SysEx
        if (packet->_data[1] != static_cast<uint8_t>(Packet::Status::SystemExclusive))
          return false;
      }

      _sysex.in.buffer[_sysex.in.length++] = packet->_data[1];
      _sysex.in.buffer[_sysex.in.length++] = packet->_data[2];
      break;

    case CodeIndex::SystemExclusiveEnd3:
      // Invalid 'End' packet
      if (packet->_data[3] != static_cast<uint8_t>(Packet::Status::SystemExclusiveEnd)) {
        _sysex.in.appending = false;
        _sysex.in.length    = 0;
        return false;
      }

      // Not enough space to store the stream
      if (_sysex.in.length + 3 > _sysex.size) {
        _sysex.in.appending = false;
        _sysex.in.length    = 0;
        return false;
      }

      // Single 'End' packet
      if (!_sysex.in.appending) {
        _sysex.in.length = 0;

        // Must be a 'one byte' SysEx
        if (packet->_data[1] != static_cast<uint8_t>(Packet::Status::SystemExclusive))
          return false;
      }

      _sysex.in.buffer[_sysex.in.length++] = packet->_data[1];
      _sysex.in.buffer[_sysex.in.length++] = packet->_data[2];
      _sysex.in.buffer[_sysex.in.length++] = packet->_data[3];
      break;

    default:
      _sysex.in.appending = false;
      _sysex.in.length    = 0;
      return false;
  }

  // Always return 'SystemExclusive' as type
  _sysex.in.appending = false;
  packet->_data[1]    = static_cast<uint8_t>(Packet::Status::SystemExclusive);
  return true;
}

void V2MIDI::Port::sendSystemExclusive(Transport *transport, uint32_t length) {
  if (length < 2)
    return;

  if (_sysex.out.buffer[0] != static_cast<uint8_t>(V2MIDI::Packet::Status::SystemExclusive))
    return;

  if (_sysex.out.buffer[length - 1] != static_cast<uint8_t>(V2MIDI::Packet::Status::SystemExclusiveEnd))
    return;

  _sysex.out.transport = transport;
  _sysex.out.length    = length;
  _sysex.out.position  = 0;

  // Send as many packets as possible.
  while (loopSystemExclusive() > 0)
    ;
}

int8_t V2MIDI::Port::loopSystemExclusive() {
  if (_sysex.out.length == 0)
    return 0;

  V2MIDI::Packet _packet;
  const uint32_t remain = _sysex.out.length - _sysex.out.position;
  switch (remain) {
    case 1:
      _packet._data[0] = (_index << 4) | static_cast<uint8_t>(CodeIndex::SystemExclusiveEnd1);
      _packet._data[1] = _sysex.out.buffer[_sysex.out.position];
      _packet._data[2] = 0;
      _packet._data[3] = 0;
      break;

    case 2:
      _packet._data[0] = (_index << 4) | static_cast<uint8_t>(CodeIndex::SystemExclusiveEnd2);
      _packet._data[1] = _sysex.out.buffer[_sysex.out.position];
      _packet._data[2] = _sysex.out.buffer[_sysex.out.position + 1];
      _packet._data[3] = 0;
      break;

    case 3:
      _packet._data[0] = (_index << 4) | static_cast<uint8_t>(CodeIndex::SystemExclusiveEnd3);
      _packet._data[1] = _sysex.out.buffer[_sysex.out.position];
      _packet._data[2] = _sysex.out.buffer[_sysex.out.position + 1];
      _packet._data[3] = _sysex.out.buffer[_sysex.out.position + 2];
      break;

    default:
      _packet._data[0] = (_index << 4) | static_cast<uint8_t>(CodeIndex::SystemExclusiveStart);
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

uint8_t V2MIDI::Packet::getPort() {
  return _data[0] >> 4;
}

void V2MIDI::Packet::setPort(uint8_t port) {
  _data[0] &= 0x0f;
  _data[0] |= port << 4;
}

uint8_t V2MIDI::Packet::getChannel() {
  return _data[1] & 0x0f;
}

void V2MIDI::Packet::setChannel(uint8_t channel) {
  _data[1] &= 0xf0;
  _data[1] |= channel;
}

V2MIDI::Packet::Status V2MIDI::Packet::getType() {
  // Remove channel number
  Status status = static_cast<Status>(_data[1] & 0xf0);
  if (status != Status::System)
    return status;

  // 'System' messages carry their message type
  return static_cast<Status>(_data[1]);
}

uint8_t V2MIDI::Packet::getNote() {
  return _data[2];
}

uint8_t V2MIDI::Packet::getNoteVelocity() {
  return _data[3];
}

uint8_t V2MIDI::Packet::getAftertouchNote() {
  return _data[2];
}

uint8_t V2MIDI::Packet::getAftertouch() {
  return _data[3];
}

uint8_t V2MIDI::Packet::getController() {
  return _data[2];
}

uint8_t V2MIDI::Packet::getControllerValue() {
  return _data[3];
}

uint8_t V2MIDI::Packet::getProgram() {
  return _data[2];
}

uint8_t V2MIDI::Packet::getAftertouchChannel() {
  return _data[2];
}

int16_t V2MIDI::Packet::getPitchBend() {
  // 14 bit -8192..8191.
  const int16_t value = _data[3] << 7 | _data[2];
  return value - 8192;
}

uint16_t V2MIDI::Packet::getSongPosition() {
  return _data[3] << 7 | _data[2];
}

uint16_t V2MIDI::Packet::getSongSelect() {
  return _data[2];
}

const uint8_t *V2MIDI::Packet::getData() {
  return _data;
}

V2MIDI::Packet *V2MIDI::Packet::setData(const uint8_t data[4]) {
  memcpy(_data, data, 4);
  return this;
}

V2MIDI::Packet *V2MIDI::Packet::set(uint8_t channel, Status type, uint8_t data1, uint8_t data2) {
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
    // the channel number.
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

V2MIDI::Packet *V2MIDI::Packet::setNote(uint8_t channel, uint8_t note, uint8_t velocity) {
  // 64 appears to be a reasonable compromise for devices which respond to NoteOff velocity.
  if (velocity == 0)
    return set(channel, Status::NoteOff, note, 64);

  return set(channel, Status::NoteOn, note, velocity);
}

V2MIDI::Packet *V2MIDI::Packet::setNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
  return set(channel, Status::NoteOff, note, velocity);
}

V2MIDI::Packet *V2MIDI::Packet::setAftertouch(uint8_t channel, uint8_t note, uint8_t pressure) {
  return set(channel, Status::Aftertouch, note, pressure);
}

V2MIDI::Packet *V2MIDI::Packet::setControlChange(uint8_t channel, uint8_t controller, uint8_t value) {
  return set(channel, Status::ControlChange, controller, value);
}

V2MIDI::Packet *V2MIDI::Packet::setAftertouchChannel(uint8_t channel, uint8_t pressure) {
  return set(channel, Status::AftertouchChannel, pressure, 0);
}

V2MIDI::Packet *V2MIDI::Packet::setProgram(uint8_t channel, uint8_t value) {
  return set(channel, Status::ProgramChange, value, 0);
}

V2MIDI::Packet *V2MIDI::Packet::setPitchBend(uint8_t channel, int16_t value) {
  // 14 bit -8192..8191.
  const uint16_t bits = value + 8192;
  return set(channel, Status::PitchBend, bits & 0x7f, (bits >> 7) & 0x7f);
}
