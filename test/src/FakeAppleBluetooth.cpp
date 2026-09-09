// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/* Stubs for the iOS Bluetooth LE support: the test programs link
   Device/Config.cpp and Device/Port/ConfiguredPort.cpp, which refer
   to these symbols on iOS (including the simulator), but the
   CoreBluetooth implementation is not part of the test binaries. */

#include "Device/Features.hpp"

#ifdef HAVE_APPLE_BLUETOOTH

#include "Apple/BluetoothHelper.hpp"
#include "Device/Port/AppleBluetoothPort.hpp"

BluetoothHelper *bluetooth_helper;

const char *
BluetoothHelper::GetNameFromAddress(const char *) const noexcept
{
  return nullptr;
}

std::unique_ptr<Port>
OpenAppleBleSerialPort(BluetoothHelper &, const char *,
                       PortListener *, DataHandler &)
{
  /* never reached: the test programs leave bluetooth_helper at
     nullptr, so ConfiguredPort fails before calling this */
  return nullptr;
}

#endif
