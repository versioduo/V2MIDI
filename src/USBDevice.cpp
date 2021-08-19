// © Kay Sievers <kay@vrfy.org>, 2020-2021
// SPDX-License-Identifier: Apache-2.0

#include "V2MIDI.h"

void V2MIDI::USBDevice::setPorts(uint8_t n_ports) {
  _device.setCables(n_ports);
}

void V2MIDI::USBDevice::begin() {
  _device.begin();
}

bool V2MIDI::USBDevice::send(Packet *midi) {
  _usec = micros();
  if (!::USBDevice.mounted())
    return false;

  return _device.writePacket(midi->_data);
}

bool V2MIDI::USBDevice::receive(Packet *midi) {
  if (!::USBDevice.mounted())
    return false;

  if (!_device.readPacket(midi->_data))
    return false;

  _usec = micros();
  return true;
}

bool V2MIDI::USBDevice::connected() {
  return ::USBDevice.mounted();
}

bool V2MIDI::USBDevice::idle() {
  return (unsigned long)(micros() - _usec) > 1000;
}
