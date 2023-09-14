// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class Port;
class PortListener;
class DataHandler;
class UsbSerialHelper;

std::unique_ptr<Port>
OpenAndroidUsbSerialPort(UsbSerialHelper &usb_serial_helper,
                         const char *name, unsigned baud,
                         PortListener *_listener, DataHandler &handler);
