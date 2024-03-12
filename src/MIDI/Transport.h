// © Kay Sievers <kay@versioduo.com>, 2020-2023
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace V2MIDI {
  class Transport {
  public:
    virtual bool receive(Packet* midi) = 0;
    virtual bool send(Packet* midi)    = 0;
  };
}
