// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Features.hpp"

#include <memory> // for std::unique_ptr
#include <utility> // for std::pair

namespace Cares { class Channel; }
class EventLoop;
class Port;
class PortListener;
class DataHandler;
struct DeviceConfig;
class SensorListener;

#ifdef HAVE_INTERNAL_GPS
class InternalSensors;
#endif

#ifdef ANDROID
namespace Java { class LocalCloseable; }
class _jobject;
class Context;
class BluetoothHelper;
class IOIOHelper;
class UsbSerialHelper;
#endif

class DeviceFactory final {
  EventLoop &event_loop;
  Cares::Channel &cares;

#ifdef ANDROID
  Context &context;
  _jobject *const permission_manager;
  BluetoothHelper *const bluetooth_helper;
  IOIOHelper *const ioio_helper;
  UsbSerialHelper *const usb_serial_helper;
#endif

public:
  constexpr DeviceFactory(EventLoop &_event_loop, Cares::Channel &_cares
#ifdef ANDROID
                          , Context &_context,
                          _jobject *_permission_manager,
                          BluetoothHelper *_bluetooth_helper,
                          IOIOHelper *_ioio_helper,
                          UsbSerialHelper *_usb_serial_helper
#endif
    ) noexcept
    :event_loop(_event_loop), cares(_cares)
#ifdef ANDROID
    , context(_context),
     permission_manager(_permission_manager),
     bluetooth_helper(_bluetooth_helper),
     ioio_helper(_ioio_helper),
     usb_serial_helper(_usb_serial_helper)
#endif
    {}

  std::unique_ptr<Port> OpenPort(const DeviceConfig &config, PortListener *listener,
                                 DataHandler &handler);

#ifdef HAVE_INTERNAL_GPS
  InternalSensors *OpenInternalSensors(SensorListener &listener);
#endif

#ifdef ANDROID
  std::pair<Java::LocalCloseable, Java::LocalCloseable> OpenDroidSoarV2(SensorListener &listener);
  Java::LocalCloseable OpenI2Cbaro(const DeviceConfig &config, SensorListener &listener);
  Java::LocalCloseable OpenNunchuck(const DeviceConfig &config, SensorListener &listener);
  Java::LocalCloseable OpenVoltage(SensorListener &listener);
  Java::LocalCloseable OpenGliderLink(SensorListener &listener);
  Java::LocalCloseable OpenBluetoothSensor(const DeviceConfig &config, SensorListener &listener);
#endif
};
