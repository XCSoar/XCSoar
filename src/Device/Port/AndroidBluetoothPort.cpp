// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AndroidBluetoothPort.hpp"
#include "AndroidPort.hpp"
#include "Android/Main.hpp"
#include "Android/BluetoothHelper.hpp"
#include "java/Global.hxx"

#include <cassert>

std::unique_ptr<Port>
OpenAndroidBluetoothPort(const TCHAR *address, PortListener *listener,
                         DataHandler &handler)
{
  assert(address != nullptr);

  if (bluetooth_helper == nullptr)
    throw std::runtime_error("Bluetooth not available");

  PortBridge *bridge = bluetooth_helper->connect(Java::GetEnv(), address);
  assert(bridge != nullptr);
  return std::make_unique<AndroidPort>(listener, handler, bridge);
}

std::unique_ptr<Port>
OpenAndroidBluetoothServerPort(PortListener *listener, DataHandler &handler)
{
  if (bluetooth_helper == nullptr)
    throw std::runtime_error("Bluetooth not available");

  PortBridge *bridge = bluetooth_helper->createServer(Java::GetEnv());
  assert(bridge != nullptr);
  return std::make_unique<AndroidPort>(listener, handler, bridge);
}

std::unique_ptr<Port>
OpenAndroidBleHm10Port(const TCHAR *address, PortListener *listener,
                         DataHandler &handler)
{
  assert(address != nullptr);

  if (bluetooth_helper == nullptr)
    throw std::runtime_error("Bluetooth not available");

  PortBridge *bridge = bluetooth_helper->connectHM10(Java::GetEnv(), address);
  assert(bridge != nullptr);
  return std::make_unique<AndroidPort>(listener, handler, bridge);
}
