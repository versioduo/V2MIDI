// © Kay Sievers <kay@versioduo.com>, 2020-2023
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CC.h"
#include "Port.h"

namespace V2MIDI::CC {
// Handle high-resolution controllers, MSB + LSB, 14 bits values.
//
// The controllers 0-31 (MSB) have matching high-resolution parts
// with controllers 32-63 (LSB).
template <uint8_t first, uint8_t size = 1> class HighResolution {
public:
  void reset() {
    memset(_controllers, 0, sizeof(_controllers));
  }

  uint16_t get(uint8_t controller = first) {
    return _controllers[controller - first].value;
  }

  uint8_t getMSB(uint8_t controller = first) {
    return (_controllers[controller - first].value >> 7);
  }

  uint8_t getLSB(uint8_t controller = first) {
    return _controllers[controller - first].value & 0x7f;
  }

  float getFraction(uint8_t controller = first) {
    return (float)_controllers[controller - first].value / 16383.f;
  }

  // Store the high-resolution value and return if the value has changed.
  bool set(uint8_t controller, uint16_t value) {
    if (value == _controllers[controller - first].value)
      return false;

    _controllers[controller - first].value = value;
    return true;
  }

  bool setFraction(uint8_t controller, float fraction) {
    return set(controller, fraction * 16383.f);
  }

  // Set MSB and LSB independently, return if the resulting high-resolution
  // value has changed.
  //
  // MIDI specification:
  // - The order is MSB, LSB.
  // - An MSB resets the current LSB.
  // - The LSB can be updated without sending the same MSB again.
  //
  // This implementation:
  // - After a reset, setting a value of 0 will not cause an update.
  // - The very first MSB causes an update without waiting for a possible LSB.
  // - If we have seen an LSB for the previuos update, we defer the update
  //   for the next MSB until the LSB arrives.
  // - If we see two MSBs without an LSB in-between, reset the waiting for
  //   the LSB and send an update.
  //   Senders are not required to send the unchanged MSB, but are expected
  //   to always send the LSB after the MSB, if high-resolution controllers
  //   are used.
  //
  // State transition:
  //   Init
  //     MSB: -> LowResolution, update
  //     LSB: discard
  //
  //   LowResolution:
  //     MSB: update
  //     LSB: -> HighResolution, update
  //
  //   HighResolution
  //     MSB: -> Wait
  //     LSB: update
  //
  //   Wait
  //     MSB: -> LowResolution, update
  //     LSB: -> HighResolution, update
  bool setByte(uint8_t controller, uint8_t value) {
    if (controller < ControllerLSB) {
      _controllers[controller - first].msb = value;

      switch (_controllers[controller - first].state) {
        // Very first MSB.
        case State::Init:
          _controllers[controller - first].state = State::LowResolution;
          break;

        // We have not seen a valid LSB for the last MSB.
        case State::LowResolution:
          break;

        // We've seen an LSB before, defer the update.
        case State::HighResolution:
          _controllers[controller - first].state = State::Wait;
          return false;

        // Two MSBs in a row, reset the high-resolution mode.
        case State::Wait:
          _controllers[controller - first].state = State::LowResolution;
          break;
      }

      const uint16_t v = value << 7;
      if (v == _controllers[controller - first].value)
        return false;

      _controllers[controller - first].value = v;
      return true;
    }

    // Ignore the LSB if we haven't seen an MSB.
    if (_controllers[controller - first - ControllerLSB].state == State::Init)
      return false;

    _controllers[controller - first - ControllerLSB].state = State::HighResolution;

    const uint16_t v = (_controllers[controller - first - ControllerLSB].msb << 7) | value;
    if (v == _controllers[controller - first - ControllerLSB].value)
      return false;

    _controllers[controller - first - ControllerLSB].value = v;
    return true;
  }

  bool send(Transport *transport, uint8_t channel, uint8_t controller) {
    Packet packet{};
    if (!transport->send(packet.setControlChange(channel, controller, getMSB(controller))))
      return false;

    return transport->send(packet.setControlChange(channel, ControllerLSB + controller, getLSB(controller)));
  }

  bool send(Port *port, uint8_t channel, uint8_t controller) {
    Packet packet{};
    if (!port->send(packet.setControlChange(channel, controller, getMSB(controller))))
      return false;

    return port->send(packet.setControlChange(channel, ControllerLSB + controller, getLSB(controller)));
  }

private:
  enum class State : uint8_t { Init, LowResolution, HighResolution, Wait };
  struct {
    State state;
    uint8_t msb;
    uint16_t value;
  } _controllers[size]{};
};
};
