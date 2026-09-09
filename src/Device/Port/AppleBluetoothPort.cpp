// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AppleBluetoothPort.hpp"
#include "ApplePort.hpp"
#include "Apple/BluetoothHelper.hpp"

#include <cassert>

std::unique_ptr<Port>
OpenAppleBleSerialPort(BluetoothHelper &bluetooth_helper,
                       const char *address, PortListener *listener,
                       DataHandler &handler)
{
  assert(address != nullptr);

  PortBridge *bridge = bluetooth_helper.connect(address);
  assert(bridge != nullptr);

  return std::make_unique<ApplePort>(listener, handler, bridge);
}
