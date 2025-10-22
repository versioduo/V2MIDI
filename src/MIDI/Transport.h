#pragma once

namespace V2MIDI {
  class Transport {
  public:
    virtual bool receive(Packet* midi) = 0;
    virtual bool send(Packet* midi)    = 0;
  };
}
