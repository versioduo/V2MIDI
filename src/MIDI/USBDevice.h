// © Kay Sievers <kay@versioduo.com>, 2020-2023
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Packet.h"
#include "Transport.h"
#include <V2Base.h>

namespace V2MIDI {
  class USBDevice : public Transport, public V2Base::USBDevice {
  public:
    bool send(Packet* midi) {
      return V2Base::USBDevice::send(midi->_data);
    }

    bool receive(Packet* midi) {
      return V2Base::USBDevice::receive(midi->_data);
    }
  };
}
