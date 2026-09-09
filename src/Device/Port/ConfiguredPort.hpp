// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Features.hpp"

#include <memory>

namespace Cares { class Channel; }
class EventLoop;
class Port;
class PortListener;
class DataHandler;
struct DeviceConfig;

#ifdef ANDROID
class BluetoothHelper;
class IOIOHelper;
class UsbSerialHelper;
#elif defined(HAVE_APPLE_BLUETOOTH)
class BluetoothHelper;
#endif

/**
 * Open the port described by #DeviceConfig.  On error, throws a
 * #std::runtime_error or returns nullptr.
 */
std::unique_ptr<Port>
OpenPort(EventLoop &event_loop, Cares::Channel &cares,
#ifdef ANDROID
         BluetoothHelper *bluetooth_helper,
         IOIOHelper *ioio_helper,
         UsbSerialHelper *usb_serial_helper,
#elif defined(HAVE_APPLE_BLUETOOTH)
         BluetoothHelper *bluetooth_helper,
#endif
         const DeviceConfig &config, PortListener *listener,
         DataHandler &handler);
