// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class BluetoothHelper;
class Port;
class PortListener;
class DataHandler;

/**
 * Open a serial port connection to a Bluetooth LE (GATT) device.
 */
std::unique_ptr<Port>
OpenAppleBleSerialPort(BluetoothHelper &bluetooth_helper,
                       const char *address, PortListener *_listener,
                       DataHandler &_handler);
