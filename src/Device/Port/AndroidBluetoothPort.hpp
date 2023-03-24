// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/Compiler.h"

#include <memory>
#include <tchar.h>

class Port;
class PortListener;
class DataHandler;

std::unique_ptr<Port>
OpenAndroidBluetoothPort(const TCHAR *address, PortListener *_listener,
                         DataHandler &_handler);

std::unique_ptr<Port>
OpenAndroidBluetoothServerPort(PortListener *_listener, DataHandler &_handler);

std::unique_ptr<Port>
OpenAndroidBleHm10Port(const TCHAR *address, PortListener *_listener,
                         DataHandler &_handler);
