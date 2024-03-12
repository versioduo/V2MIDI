// © Kay Sievers <kay@versioduo.com>, 2023
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Packet.h"
#include <V2Base.h>

namespace V2MIDI::File {

  // The event in a MIDI track.
  class Event {
  public:
    enum class Meta : uint8_t {
      Sequence      = 0x00,
      Text          = 0x01,
      Copyright     = 0x02,
      Title         = 0x03,
      Instrument    = 0x04,
      Lyric         = 0x05,
      Marker        = 0x06,
      CuePoint      = 0x07,
      ProgramName   = 0x08,
      DeviceName    = 0x09,
      Channel       = 0x20,
      Port          = 0x21,
      EndOfTrack    = 0x2f,
      Tempo         = 0x51,
      SmpteOffset   = 0x54,
      TimeSignature = 0x58,
      KeySignature  = 0x59,
      Sequencer     = 0x7f,
    };

    // The delay in ticks until the event fires. A zero delta value means that
    // multiple events in the same stream fire at the same time.
    uint32_t delta;

    enum class Type { None, Meta, SysEx, Message } type;
    union {
      Meta    metaType;
      uint8_t sysExType;
      struct {
        Packet::Status status;
        uint8_t        channel;
      };
    };

    const uint8_t* data;
    uint32_t       length;
  };

  // The track in a MIDI file, it contains the events.
  class Track {
  public:
    const uint8_t* data;
    uint32_t       length;

    // Find a specific meta tag in the track and return its data.
    ssize_t copyTag(Event::Meta meta, char* text, uint32_t size) {
      uint32_t cursor = 0;

      for (;;) {
        Event e;
        if (!readEvent(e, cursor))
          break;

        if (e.type != Event::Type::Meta)
          continue;

        if (e.metaType != meta)
          continue;

        if (e.length + 1 > size)
          return -1;

        memcpy(text, e.data, e.length);
        text[e.length] = '\0';
        return e.length;
      }

      return 0;
    }

    // Iterate over the stream of events in a track.
    bool readEvent(Event& e, uint32_t& cursor) {
      if (cursor >= length) {
        e.type = Event::Type::None;
        return false;
      }

      e.delta = readNumber(cursor);

      switch (data[cursor]) {
        case 0xff:
          cursor++;
          e.type     = Event::Type::Meta;
          e.metaType = (Event::Meta)data[cursor++];
          e.length   = readNumber(cursor);
          e.data     = data + cursor;
          cursor += e.length;
          if (e.metaType == Event::Meta::EndOfTrack) {
            e.type = Event::Type::None;
            return false;
          }
          return true;

        case 0xf0:
        case 0xf7:
          e.type      = Event::Type::SysEx;
          e.sysExType = data[cursor++];
          e.length    = readNumber(cursor);
          e.data      = data + cursor;
          cursor += e.length;
          return true;

        default:
          e.type = Event::Type::Message;
          if (data[cursor] >= 0x80) {
            if (static_cast<Packet::Status>(data[cursor] & 0xf0) != Packet::Status::System) {
              e.status  = static_cast<Packet::Status>(data[cursor] & 0xf0);
              e.channel = data[cursor] & 0x0f;

            } else {
              e.status  = static_cast<Packet::Status>(data[cursor]);
              e.channel = 0;
            }

            cursor++;
            _running.status  = e.status;
            _running.channel = e.channel;

          } else {
            e.status  = _running.status;
            e.channel = _running.channel;
          }

          switch (e.status) {
            case Packet::Status::NoteOn:
            case Packet::Status::NoteOff:
            case Packet::Status::Aftertouch:
            case Packet::Status::ControlChange:
            case Packet::Status::PitchBend:
            case Packet::Status::SystemSongPosition:
              e.length = 2;
              break;

            case Packet::Status::ProgramChange:
            case Packet::Status::AftertouchChannel:
            case Packet::Status::SystemTimeCodeQuarterFrame:
            case Packet::Status::SystemSongSelect:
              e.length = 1;
              break;

            case Packet::Status::SystemTuneRequest:
            case Packet::Status::SystemClock:
            case Packet::Status::SystemStart:
            case Packet::Status::SystemContinue:
            case Packet::Status::SystemStop:
            case Packet::Status::SystemActiveSensing:
            case Packet::Status::SystemReset:
              e.length = 0;
              break;
          }

          e.data = data + cursor;
          cursor += e.length;
          return true;
      }
    }

  private:
    // MIDI Running status. Repeated channel messages of the same type and channel might omit
    // the leading status byte.
    struct {
      Packet::Status status;
      uint8_t        channel;
    } _running;

    // Read a variable-length encoded number. Big Endian, 7 bit data / byte.
    uint32_t readNumber(uint32_t& cursor) const {
      uint32_t number = 0;

      for (;;) {
        const uint8_t b = data[cursor++];
        number |= b & 0x7f;
        if (b < 0x80)
          break;

        number <<= 7;
      }

      return number;
    }
  };

  // The MIDI file, it contains the tracks.
  class Tracks {
  public:
    enum class State { Empty, Loaded, Play, Stop };

    constexpr Tracks(){};
    constexpr Tracks(const uint8_t* data = NULL) {
      load(data);
    }

    constexpr bool load(const uint8_t* data = NULL) {
      if (!data) {
        if (_state != State::Empty) {
          _state = State::Empty;
          handleStateChange(_state);
        }

        return false;
      }

      _state          = State::Empty;
      _data           = data;
      uint32_t cursor = 0;

      if (!readSignature("MThd", cursor))
        return false;

      if (readBE32(cursor) != 6)
        return false;

      // 0: Single multi-channel track
      // 1: One or more simultaneous tracks/outputs
      // 2: One or more sequentially independent single-track patterns
      //
      // Do not bother with version 3, it is not worth to support tracking
      // separate a separate tempo for every track; independent tracks are
      // preferred as separate files.
      _header.version = readBE16(cursor);
      if (_header.version > 1)
        return false;

      // The number of tracks in the file.
      _header.nTracks = readBE16(cursor);
      if (_header.nTracks > _maxTracks)
        return false;

      // The ticks per beat.
      _header.division = readBE16(cursor);

      // Bit 15 is SMPTE format.
      if (_header.division & 0x8000)
        return false;

      for (uint16_t i = 0; i < _header.nTracks; i++) {
        if (!readSignature("MTrk", cursor))
          return false;

        uint32_t length = readBE32(cursor);
        if (length < 2)
          return false;

        _tracks[i].data   = _data + cursor;
        _tracks[i].length = length;
        cursor += length;
      }

      _state = State::Loaded;
      handleStateChange(_state);
      return true;
    }

    int16_t getFormat() const {
      if (_state == State::Empty)
        return -1;

      return _header.version;
    }

    int16_t getTrackCount() const {
      if (_state == State::Empty)
        return -1;

      return _header.nTracks;
    }

    const Track* getTrack(uint16_t track) const {
      if (_state == State::Empty)
        return NULL;

      if (track >= _header.nTracks)
        return NULL;

      return &_tracks[track];
    }

    // Find a specific meta tag in track 0.
    ssize_t copyTag(Event::Meta meta, char* text, uint32_t size) {
      if (_state == State::Empty)
        return -1;

      return _tracks[0].copyTag(meta, text, size);
    }

    bool play() {
      if (_state == State::Empty)
        return false;

      for (uint8_t i = 0; i < _header.nTracks; i++)
        _play.tracks[i] = {};

      // The default tempo, if no tempo events are in track 0.
      setTempoBPM(120);

      _play.tick     = 0;
      _play.lastUsec = V2Base::getUsec();

      _state = State::Play;
      handleStateChange(_state);
      return true;
    }

    void stop() {
      if (_state != State::Play)
        return;

      _state = State::Stop;
      handleStateChange(_state);
    }

    // This needs to be called from a few times a millisecond to every
    // few milliseconds. The playback speed does not depend on the call
    // frequency, it only affects the accuracy of the events timing.
    void run() {
      if (_state != State::Play)
        return;

      // Calculate the time since the last run.
      const uint32_t nowUsec    = V2Base::getUsec();
      const uint32_t passedUsec = (uint32_t)(nowUsec - _play.lastUsec);
      _play.lastUsec            = nowUsec;

      // Add the number of ticks which have passed since the last run.
      _play.tick += passedUsec / _play.tickDurationUsec;

      bool playing{};

      for (uint8_t i = 0; i < _header.nTracks; i++) {
        if (_play.tracks[i].end)
          continue;

        playing = true;

        // Check if the current track has pending messages.
        if (_play.tick < _play.tracks[i].tick)
          continue;

        const Event* e = &_play.tracks[i].event;
        for (;;) {
          // Read a new event, or handle the previous / delayed event.
          if (e->type == Event::Type::None) {
            if (!_tracks[i].readEvent(_play.tracks[i].event, _play.tracks[i].cursor)) {
              _play.tracks[i].end = true;
              break;
            }

            if (e->delta > 0) {
              // Delay event.
              _play.tracks[i].tick += e->delta;
              break;
            }
          }

          // Track 0 might change the global playback tempo.
          if (i == 0 && e->type == Event::Type::Meta && e->metaType == Event::Meta::Tempo) {
            // 24 bit integer, the number of microseconds per beat. Updates the global tempo.
            setTempoUsec(e->data[0] << 16 | e->data[1] << 8 | e->data[2]);
            _play.tracks[i].event.type = Event::Type::None;
            continue;
          }

          if (e->type == Event::Type::Message) {
            Packet midi;

            switch (e->status) {
              case Packet::Status::NoteOn:
              case Packet::Status::NoteOff:
              case Packet::Status::Aftertouch:
              case Packet::Status::ControlChange:
              case Packet::Status::PitchBend:
                handleSend(i, midi.set(e->channel, e->status, e->data[0], e->data[1]));
                break;

              case Packet::Status::ProgramChange:
              case Packet::Status::AftertouchChannel:
                handleSend(i, midi.set(e->channel, e->status, e->data[0]));
                break;
            }
          }

          _play.tracks[i].event.type = Event::Type::None;
        }
      }

      if (!playing) {
        _state = State::Stop;
        handleStateChange(_state);
      }
    }

    // Used if run() is not called periodically from a timer.
    void loop() {
      if (V2Base::getUsecSince(_usec) < 1000)
        return;

      _usec = V2Base::getUsec();

      run();
    }

  protected:
    // Notify about Start, Stop / the end of playback.
    virtual void handleStateChange(State state) {}

    // Send MIDI packets.
    virtual bool handleSend(uint16_t track, Packet* packet) {
      return false;
    }

  private:
    static constexpr uint16_t _maxTracks{16};
    State                     _state{};
    uint32_t                  _usec{};

    // The loaded MIDI file.
    const uint8_t* _data{};

    struct {
      uint16_t version;
      uint16_t nTracks;
      uint16_t division;
    } _header{};
    Track _tracks[_maxTracks]{};

    // The global tempo and track state during playback.
    struct {
      // The duration of one MIDI tick.
      float tickDurationUsec{};

      // The current tick while playing the file.
      float tick{};

      // The last time the tick handler was called.
      uint32_t lastUsec{};

      // The played tracks.
      struct {
        uint32_t cursor;
        float    tick;
        Event    event;
        bool     end;
      } tracks[_maxTracks];
    } _play{};

    // Read a 4 byte section / chunk header.
    bool readSignature(const char signature[4], uint32_t& cursor) const {
      const uint8_t* header = _data + cursor;
      cursor += 4;
      return memcmp(header, signature, 4) == 0;
    }

    // Read Big Endian integers.
    uint32_t readBE32(uint32_t& cursor) const {
      uint32_t be32;
      memcpy(&be32, _data + cursor, 4);
      cursor += 4;
      return __builtin_bswap32(be32);
    }

    uint16_t readBE16(uint32_t& cursor) const {
      uint32_t be16;
      memcpy(&be16, _data + cursor, 2);
      cursor += 2;
      return __builtin_bswap16(be16);
    }

    void setTempoBPM(float bpm) {
      const float usec = (60.f * 1000.f * 1000.f) / bpm;
      setTempoUsec(usec);
    }

    void setTempoUsec(float usec) {
      _play.tickDurationUsec = usec / (float)_header.division;
    }
  };
}
