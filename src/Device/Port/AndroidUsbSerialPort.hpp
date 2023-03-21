// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class Port;
class PortListener;
class DataHandler;

std::unique_ptr<Port>
OpenAndroidUsbSerialPort(const char *name, unsigned baud,
                         PortListener *_listener, DataHandler &handler);
