// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/Compiler.h"

#include <memory>
#include <tchar.h>

class BluetoothHelper;
class Port;
class PortListener;
class DataHandler;

std::unique_ptr<Port>
OpenAndroidBluetoothPort(BluetoothHelper &bluetooth_helper,
                         const char *address, PortListener *_listener,
                         DataHandler &_handler);

std::unique_ptr<Port>
OpenAndroidBluetoothServerPort(BluetoothHelper &bluetooth_helper,
                               PortListener *_listener, DataHandler &_handler);

std::unique_ptr<Port>
OpenAndroidBleHm10Port(BluetoothHelper &bluetooth_helper,
                       const char *address, PortListener *_listener,
                       DataHandler &_handler);
