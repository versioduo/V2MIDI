// © Kay Sievers <kay@versioduo.com>, 2020-2023
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>

namespace V2MIDI {
  // MIDI clock/sync.
  class Clock {
  public:
    enum class Event {
      Tick,
      Start,
      Continue,
      Stop,
    };

    void reset() {
      _run  = false;
      _tick = 0;
    }

    uint32_t getTick() {
      return _tick;
    }

    // Song Position sets the number of beats.
    void setBeat(uint32_t beat) {
      _tick = beat * 6;
    }

    uint32_t getBeat() {
      return _tick / 6;
    }

    uint32_t getQuarter() {
      return _tick / 24;
    }

    void update(Event clock) {
      switch (clock) {
        case Event::Tick:
          // Sent at a rate of 24 per quarter note.
          if (!_run)
            break;

          if ((_tick % 24) == 0)
            handleQuarter(_tick / 24);
          _tick++;
          break;

        case Event::Start:
          _run  = true;
          _tick = 0;
          break;

        case Event::Continue:
          // A sequence will continue from its current location upon receipt of the next tick.
          _run = true;
          break;

        case Event::Stop:
          _run = false;
          break;
      }
    }

  protected:
    virtual void handleQuarter(uint32_t beat) {}

  private:
    bool     _run{};
    uint32_t _tick{};
  };
}
