// © Kay Sievers <kay@vrfy.org>, 2020-2021
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <Adafruit_TinyUSB.h>
#include <Arduino.h>

namespace V2MIDI {
class Port;

class Packet {
public:
  // MIDI status byte (bit 4 to 7, bit 7 is always set).
  enum class Status {
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

  enum class Clock {
    Tick,
    Start,
    Continue,
    Stop,
  };

  // Set virtual port/wire in the packet. Port 1 == 0.
  uint8_t getPort();
  void setPort(uint8_t port);

  // Channel 1 == 0
  uint8_t getChannel();
  void setChannel(uint8_t channel);

  Status getType();

  uint8_t getNote();
  uint8_t getNoteVelocity();

  uint8_t getAftertouchNote();
  uint8_t getAftertouch();

  uint8_t getController();
  uint8_t getControllerValue();

  uint8_t getProgram();
  uint8_t getAftertouchChannel();
  int16_t getPitchBend();

  uint16_t getSongPosition();
  uint16_t getSongSelect();

  const uint8_t *getData();
  Packet *setData(const uint8_t data[4]);

  // Encode values into the packet and return is own pointer to allow the
  // stacking of function calls.
  Packet *set(uint8_t channel, Status type, uint8_t data1 = 0, uint8_t data2 = 0);
  Packet *setNote(uint8_t channel, uint8_t note, uint8_t velocity);
  Packet *setNoteOff(uint8_t channel, uint8_t note, uint8_t velocity = 64);
  Packet *setAftertouch(uint8_t channel, uint8_t note, uint8_t pressure);
  Packet *setControlChange(uint8_t channel, uint8_t controller, uint8_t value = 0);
  Packet *setProgram(uint8_t channel, uint8_t value);
  Packet *setAftertouchChannel(uint8_t channel, uint8_t pressure);
  Packet *setPitchBend(uint8_t channel, int16_t value);

private:
  friend class Port;
  friend class SerialDevice;
  friend class USBDevice;
  uint8_t _data[4]{};
};

class Transport {
public:
  virtual bool receive(Packet *midi) = 0;
  virtual bool send(Packet *midi)    = 0;
};

// Transport-independent MIDI functional interface. Supports message parsing/dispatching,
// system exclusive buffering/streaming, packet statistics.
class Port {
public:
  struct Counter {
    uint32_t packet;
    uint32_t note;
    uint32_t note_off;
    uint32_t aftertouch;
    uint32_t control;
    uint32_t program;
    uint32_t aftertouch_channel;
    uint32_t pitchbend;
    struct {
      struct {
        uint32_t tick;
      } clock;
      uint32_t exclusive;
      uint32_t reset;
    } system;
  };

  constexpr Port(uint8_t index, uint32_t sysex_size) : _index{index}, _sysex{.size{sysex_size}} {}

  void begin();

  // During dispatch(), replies can be sent back to the given 'transport'.
  void dispatch(Transport *transport, Packet *packet);

  // Set the port's number in the outgoing packet and updates the statistics.
  bool send(Packet *packet);

  // Get the raw buffer to copy the SysEx message into.
  uint8_t *getSystemExclusiveBuffer() {
    return _sysex.out.buffer;
  }

  // Prepare SysEx message to chunk into packets. Send as many packets as possible,
  // the remaining packets will be sent with loopSystemExclusive().
  void sendSystemExclusive(Transport *transport, uint32_t length);

  // Send the next packet over the specified transport. Returns:
  //  0: nothing to do,
  // -1: sending failed,
  //  1: there are remaining packets.
  int8_t loopSystemExclusive();

protected:
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
  virtual void handleClock(Packet::Clock clock) {}
  virtual void handleSystemExclusive(const uint8_t *buffer, uint32_t len) {}
  virtual void handleSystemReset() {}
  virtual void handleSwitchChannel(uint8_t channel) {}

  // All messages besides system exclusive.
  virtual void handlePacket(Packet *packet) {}

  // During dispatch, replies are sent back to the originating transport.
  virtual void handleSystemExclusive(Transport *transport, const uint8_t *buffer, uint32_t len) {}

  virtual bool handleSend(Packet *packet) {
    return false;
  };

private:
  const uint8_t _index;

  struct {
    uint32_t size;

    struct {
      uint8_t *buffer;
      uint32_t length;
      bool appending;
    } in;

    struct {
      Transport *transport;
      uint8_t *buffer;
      uint32_t length;
      uint32_t position;
    } out;
  } _sysex{};

  bool storeSystemExclusive(Packet *packet);
};

class USBDevice : public Transport {
public:
  void setPorts(uint8_t n_ports);
  void begin();
  bool send(Packet *midi);
  bool receive(Packet *midi);
  bool connected();
  bool idle();

private:
  Adafruit_USBD_MIDI _device{};
  unsigned long _usec{};
};

class SerialDevice : public Transport {
public:
  constexpr SerialDevice(Uart *uart) : _uart(uart) {}
  void begin();
  bool send(Packet *midi);
  bool receive(Packet *midi);

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

// Controller (CC) numbers and Channel Mode Messages.
namespace CC {
  enum {
    // MSB Controller Data.
    BankSelect       = 0,
    ModulationWheel  = 1,
    BreathController = 2,
    Controller3      = 3,
    FootController   = 4,
    PortamentoTime   = 5,
    DataEntry        = 6, // RPN, NRPN value.
    ChannelVolume    = 7,
    Balance          = 8,
    Controller9      = 9,
    Pan              = 10,
    Expression       = 11,
    EffectControl1   = 12,
    EffectControl2   = 13,
    Controller14     = 14,
    Controller15     = 15,
    GeneralPurpose1  = 16,
    GeneralPurpose2  = 17,
    GeneralPurpose3  = 18,
    GeneralPurpose4  = 19,
    Controller20     = 20,
    Controller21     = 21,
    Controller22     = 22,
    Controller23     = 23,
    Controller24     = 24,
    Controller25     = 25,
    Controller26     = 26,
    Controller27     = 27,
    Controller28     = 28,
    Controller29     = 29,
    Controller30     = 30,
    Controller31     = 31,

    // LSB for controllers 0 to 31.
    ControllerLSB       = 32,
    BankSelectLSB       = ControllerLSB + BankSelect,
    ModulationWheelLSB  = ControllerLSB + ModulationWheel,
    BreathControllerLSB = ControllerLSB + BreathController,
    Controller3LSB      = ControllerLSB + Controller3,
    FootControllerLSB   = ControllerLSB + FootController,
    PortamentoTimeLSB   = ControllerLSB + PortamentoTime,
    DataEntryLSB        = ControllerLSB + DataEntry,
    ChannelVolumeLSB    = ControllerLSB + ChannelVolume,
    BalanceLSB          = ControllerLSB + Balance,
    Controller9LSB      = ControllerLSB + Controller9,
    PanLSB              = ControllerLSB + Pan,
    ExpressionLSB       = ControllerLSB + Expression,
    EffectControl1LSB   = ControllerLSB + EffectControl1,
    EffectControl2LSB   = ControllerLSB + EffectControl2,
    Controller14LSB     = ControllerLSB + Controller14,
    Controller15LSB     = ControllerLSB + Controller15,
    GeneralPurpose1LSB  = ControllerLSB + GeneralPurpose1,
    GeneralPurpose2LSB  = ControllerLSB + GeneralPurpose2,
    GeneralPurpose3LSB  = ControllerLSB + GeneralPurpose3,
    GeneralPurpose4LSB  = ControllerLSB + GeneralPurpose4,
    Controller20LSB     = ControllerLSB + Controller20,
    Controller21LSB     = ControllerLSB + Controller21,
    Controller22LSB     = ControllerLSB + Controller22,
    Controller23LSB     = ControllerLSB + Controller23,
    Controller24LSB     = ControllerLSB + Controller24,
    Controller25LSB     = ControllerLSB + Controller25,
    Controller26LSB     = ControllerLSB + Controller26,
    Controller27LSB     = ControllerLSB + Controller27,
    Controller28LSB     = ControllerLSB + Controller28,
    Controller29LSB     = ControllerLSB + Controller29,
    Controller30LSB     = ControllerLSB + Controller30,
    Controller31LSB     = ControllerLSB + Controller31,

    // Single-byte Controllers.
    Sustain           = 64,
    Portamento        = 65,
    Sostenuto         = 66,
    Soft              = 67,
    Legato            = 68,
    Hold2             = 69,
    SoundController1  = 70, // Sound Variation
    SoundController2  = 71, // Timber / Harmonic Intensity
    SoundController3  = 72, // Release Time
    SoundController4  = 73, // Attack Time
    SoundController5  = 74, // Brightness
    SoundController6  = 75, // Decay Time
    SoundController7  = 76, // Vibrato Rate
    SoundController8  = 77, // Vibrato Depth
    SoundController9  = 78, // Vibrato Delay
    SoundController10 = 79,
    GeneralPurpose5   = 80, // Decay
    GeneralPurpose6   = 81, // High Pass Filter Frequency
    GeneralPurpose7   = 82,
    GeneralPurpose8   = 83,
    PortamentoControl = 84,
    Controller85      = 85,
    Controller86      = 86,
    Controller87      = 87,
    VelocityPrefix    = 88,
    Controller89      = 89,
    Controller90      = 90,
    Effects1          = 91, // Reverb Send
    Effects2          = 92, // Tremolo Depth
    Effects3          = 93, // Chorus Send
    Effects4          = 94, // Celeste Depth
    Effects5          = 95, // Phaser Depth

    // Non-registered, Registered Parameter Numbers
    DataIncrement = 96, // Step == 1, ignore the value (RP-018).
    DataDecrement = 97,
    NRPNLSB       = 98, // Select NRPN.
    NRPNMSB       = 99,
    RPNLSB        = 100, // Select RPN.
    RPNMSB        = 101,

    Controller102 = 102,
    Controller103 = 103,
    Controller104 = 104,
    Controller105 = 105,
    Controller106 = 106,
    Controller107 = 107,
    Controller108 = 108,
    Controller109 = 109,
    Controller110 = 110,
    Controller111 = 111,
    Controller112 = 112,
    Controller113 = 113,
    Controller114 = 114,
    Controller115 = 115,
    Controller116 = 116,
    Controller117 = 117,
    Controller118 = 118,
    Controller119 = 119,

    // Channel Mode Message
    AllSoundOff         = 120,
    ResetAllControllers = 121,
    LocalControl        = 122,
    AllNotesOff         = 123,
    OmniModeOff         = 124,
    OmniModeOn          = 125,
    MonoModeOn          = 126,
    PolyModeOn          = 127
  };

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

// MIDI clock/sync.
class Clock {
public:
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

  void update(Packet::Clock clock) {
    switch (clock) {
      case Packet::Clock::Tick:
        // Sent at a rate of 24 per quarter note.
        if (!_run)
          break;

        if ((_tick % 24) == 0)
          handleQuarter(_tick / 24);
        _tick++;
        break;

      case Packet::Clock::Start:
        _run  = true;
        _tick = 0;
        break;

      case Packet::Clock::Continue:
        // A sequence will continue from its current location upon receipt of the next tick.
        _run = true;
        break;

      case Packet::Clock::Stop:
        _run = false;
        break;
    }
  }

protected:
  virtual void handleQuarter(uint32_t beat) {}

private:
  bool _run{};
  uint32_t _tick{};
};

// Registered Parameter Numbers.
namespace RPN {
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

namespace GM {
  // MIDI Program Change numbers / instruments.
  namespace Program {
    enum {
      // Piano
      AcousticGrandPiano  = 0,
      BrightAcousticPiano = 1,
      ElectricGrandPiano  = 2,
      HonkyTonkPiano      = 3,
      ElectricPiano1      = 4,
      ElectricPiano2      = 5,
      Harpsichord         = 6,
      Clavi               = 7,

      // Chromatic Percussion
      celesta      = 8,
      Glockenspiel = 9,
      MusicBox     = 10,
      Vibraphone   = 11,
      Marimba      = 12,
      Xylophone    = 13,
      TubularBells = 14,
      Dulcimer     = 15,

      // Organ
      DrawbarOrgan    = 16,
      PercussiveOrgan = 17,
      RockOrgan       = 18,
      ChurchOrgan     = 19,
      ReedOrgan       = 20,
      Accordion       = 21,
      Harmonica       = 22,
      TangoAccordion  = 23,

      // Guitar
      AcousticGuitarNylon  = 24,
      AcousticGuitarSsteel = 25,
      ElectricGuitarJazz   = 26,
      ElectricGuitarClean  = 27,
      ElectricGuitarMuted  = 28,
      OverdrivenGuitar     = 29,
      DistortionGuitar     = 30,
      GuitarHarmonics      = 31,

      // Bass
      AcousticBass       = 32,
      ElectricBassFinger = 33,
      ElectricBassPick   = 34,
      FretlessBass       = 35,
      SlapBass1          = 36,
      SlapBass2          = 37,
      SynthBass1         = 38,
      SynthBass2         = 39,

      // Strings
      Violin           = 40,
      Viola            = 41,
      Cello            = 42,
      Contrabass       = 43,
      TremoloStrings   = 44,
      PizzicatoStrings = 45,
      OrchestralHarp   = 46,

      // Ensemble
      Timpani         = 47,
      StringEnsemble1 = 48,
      StringEnsemble2 = 49,
      SynthStrings1   = 50,
      SynthStrings2   = 51,
      ChoirAahs       = 52,
      VoiceOohs       = 53,
      SynthVoice      = 54,
      OrchestraHit    = 55,

      // Brass
      Trumpet      = 56,
      Trombone     = 57,
      Tuba         = 58,
      MutedTrumpet = 59,
      FrenchHorn   = 60,
      BrassSection = 61,
      SynthBrass1  = 62,
      SynthBrass2  = 63,

      // Reed
      SopranoSax  = 64,
      AltoSax     = 65,
      TenorSax    = 66,
      BaritoneSax = 67,
      Oboe        = 68,
      EnglishHorn = 69,
      Bassoon     = 70,
      Clarinet    = 71,

      // Pipe
      Piccolo     = 72,
      Flute       = 73,
      Recorder    = 74,
      PanFlute    = 75,
      BlownBottle = 76,
      Shakuhachi  = 77,
      Whistle     = 78,
      Ocarina     = 79,

      // Synth Lead
      Lead1Square   = 80,
      Lead2Sawtooth = 81,
      Lead3Calliope = 82,
      Lead4Chiff    = 83,
      Lead5Charang  = 84,
      Lead6Voice    = 85,
      Lead7Ffifths  = 86,
      Lead8Bass     = 87,

      // Synth Pad
      Pad1NewAge    = 88,
      Pad2Warm      = 89,
      Pad3Polysynth = 90,
      Pad4Choir     = 91,
      Pad5Bowed     = 92,
      Pad6Metallic  = 93,
      Pad7Halo      = 94,
      Pad8Sweep     = 95,

      // Synth Effects
      FX1Rain       = 96,
      FX2Soundtrack = 97,
      FX3Crystal    = 98,
      FX4Atmosphere = 99,
      FX5Brightness = 100,
      FX6Goblins    = 101,
      FX7Echoes     = 102,
      FX8SciFi      = 103,

      // Ethnic Percussive
      Sitar    = 104,
      Banjo    = 105,
      Shamisen = 106,
      Koto     = 107,
      Kalimba  = 108,
      BagPipe  = 109,
      Fiddle   = 110,
      Shanai   = 111,

      // Percussive
      TinkleBell    = 112,
      Agogo         = 113,
      SteelDrums    = 114,
      Woodblock     = 115,
      TaikoDrum     = 116,
      MelodicTom    = 117,
      SynthDrum     = 118,
      ReverseCymbal = 119,

      // Sound Effects
      GuitarFretNoise = 120,
      BreathNoise     = 121,
      Seashore        = 122,
      BirdTweet       = 123,
      TelephoneRing   = 124,
      Helicopter      = 125,
      Applause        = 126,
      Gunshot         = 127
    };
  };

  // General MIDI percussion mapping. Traditionally on MIDI channel 10.
  namespace Percussion {
    enum {
      HighQ            = 27,
      Slap             = 28,
      ScratchPush      = 29,
      ScratchPull      = 30,
      Sticks           = 31,
      SquareClick      = 32,
      MetronomeClick   = 33,
      MetronomeBell    = 34,
      AcousticBassDrum = 35,
      BassDrum1        = 36,
      SideStick        = 37,
      AcousticSnare    = 38,
      HandClap         = 39,
      ElectricSnare    = 40,
      LowFloorTom      = 41,
      ClosedHiHat      = 42,
      HighFloorTom     = 43,
      PedalHiHat       = 44,
      LowTom           = 45,
      OpenHiHat        = 46,
      LowMidTom        = 47,
      HiMidTom         = 48,
      CrashCymbal1     = 49,
      HighTom          = 50,
      RideCymbal1      = 51,
      ChineseCymbal    = 52,
      RideBell         = 53,
      Tambourine       = 54,
      SplashCymbal     = 55,
      Cowbell          = 56,
      CrashCymbal2     = 57,
      Vibraslap        = 58,
      RideCymbal2      = 59,
      HiBongo          = 60,
      LowBongo         = 61,
      MuteHiConga      = 62,
      OpenHiConga      = 63,
      LowConga         = 64,
      HighTimbale      = 65,
      LowTimbale       = 66,
      HighAgogo        = 67,
      LowAgogo         = 68,
      Cabasa           = 69,
      Maracas          = 70,
      ShortWhistle     = 71,
      LongWhistle      = 72,
      ShortGuiro       = 73,
      LongGuiro        = 74,
      Claves           = 75,
      HiWoodBlock      = 76,
      LowWoodBlock     = 77,
      MuteCuica        = 78,
      OpenCuica        = 79,
      MuteTriangle     = 80,
      OpenTriangle     = 81,
      Shaker           = 82,
      JingleBell       = 83,
      BellTree         = 84,
      Castanets        = 85,
      MuteSurdo        = 86,
      OpenSurdo        = 87,
    };
  };
};

// The octave numbers -2 to 8 are not defined by MIDI itself, it's just what
// some vendors of instruments and audio workstation software use. The middle
// C (MIDI Note 60) in this mapping is C(3).
static constexpr int8_t octaveOfMiddleC = 3;
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
