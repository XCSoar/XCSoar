// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AppleBluetoothPort.hpp"
#include "ApplePort.hpp"
#include "Apple/BluetoothHelper.hpp"

#include <cassert>

std::unique_ptr<Port>
OpenAppleBluetoothPort(BluetoothHelper &bluetooth_helper,
                         const TCHAR *address, PortListener *listener,
                         DataHandler &handler)
{
  assert(address != nullptr);

  PortBridge *bridge = bluetooth_helper.connect(address);
  assert(bridge != nullptr);
  return std::make_unique<ApplePort>(listener, handler, bridge);
}

std::unique_ptr<Port>
OpenAppleBluetoothServerPort(BluetoothHelper &bluetooth_helper,
                               PortListener *listener, DataHandler &handler)
{
  PortBridge *bridge = bluetooth_helper.createServer();
  assert(bridge != nullptr);
  return std::make_unique<ApplePort>(listener, handler, bridge);
}

std::unique_ptr<Port>
OpenAppleBleHm10Port(BluetoothHelper &bluetooth_helper,
                       const TCHAR *address, PortListener *listener,
                       DataHandler &handler)
{
  assert(address != nullptr);

  PortBridge *bridge = bluetooth_helper.connectHM10(address);
  assert(bridge != nullptr);
  return std::make_unique<ApplePort>(listener, handler, bridge);
}
