// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

// 20070413:sgi add NmeaOut support, allow nmea chaining an double port platforms

#include "device.hpp"
#include "Features.hpp"
#include "Device/MultipleDevices.hpp"
#include "Device/Descriptor.hpp"
#include "LogFile.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "SystemSettings.hpp"

static void
devInitOne(DeviceDescriptor &device, const DeviceConfig &config)
{
  device.SetConfig(config);
  device.ResetFailureCounter();
}

/**
 * Checks if the two configurations overlap, i.e. they request access
 * to an exclusive resource, like the same physical COM port.  If this
 * is detected, then the second device will be disabled.
 */
[[gnu::pure]]
static bool
DeviceConfigOverlaps(const DeviceConfig &a, const DeviceConfig &b)
{
  if (a.port_type != b.port_type)
    return false;

  switch (a.port_type) {
  case DeviceConfig::PortType::SERIAL:
  case DeviceConfig::PortType::PTY:
  case DeviceConfig::PortType::ANDROID_USB_SERIAL:
    return a.path.equals(b.path);

  case DeviceConfig::PortType::RFCOMM:
  case DeviceConfig::PortType::BLE_HM10:
  case DeviceConfig::PortType::BLE_SENSOR:
    return a.bluetooth_mac.equals(b.bluetooth_mac);

  case DeviceConfig::PortType::IOIOUART:
    return a.ioio_uart_id == b.ioio_uart_id;

  case DeviceConfig::PortType::I2CPRESSURESENSOR:
    return a.i2c_bus == b.i2c_bus && a.i2c_addr == b.i2c_addr;

  case DeviceConfig::PortType::DISABLED:
  case DeviceConfig::PortType::AUTO:
  case DeviceConfig::PortType::INTERNAL:
  case DeviceConfig::PortType::DROIDSOAR_V2:
  case DeviceConfig::PortType::RFCOMM_SERVER:
  case DeviceConfig::PortType::NUNCHUCK: // Who wants 2 nunchucks ??
  case DeviceConfig::PortType::IOIOVOLTAGE:
  case DeviceConfig::PortType::GLIDER_LINK:
    return true;

  case DeviceConfig::PortType::TCP_CLIENT:
    return a.tcp_port == b.tcp_port &&
      a.ip_address == b.ip_address;

  case DeviceConfig::PortType::TCP_LISTENER:
  case DeviceConfig::PortType::UDP_LISTENER:
    return a.tcp_port == b.tcp_port;
  }

  gcc_unreachable();
}

template<typename I>
[[gnu::pure]]
static bool
DeviceConfigOverlaps(const DeviceConfig &config, I begin, I end)
{
  return std::any_of(begin, end,
                     [&config](const DeviceDescriptor *d) {
                       return DeviceConfigOverlaps(config, d->GetConfig());
                     });
}

void
devStartup(MultipleDevices &devices, const SystemSettings &settings)
{
  LogString("Register serial devices");

  bool none_available = true;
  for (unsigned i = 0; i < NUMDEV; ++i) {
    DeviceDescriptor &device = devices[i];
    const DeviceConfig &config = settings.devices[i];
    if (!config.IsAvailable()) {
      device.ClearConfig();
      continue;
    }

    none_available = false;

    if (DeviceConfigOverlaps(config, devices.begin(), devices.begin() + i)) {
      device.ClearConfig();
      continue;
    }

    devInitOne(device, config);
  }

  if (none_available) {
#ifdef HAVE_INTERNAL_GPS
    /* fall back to built-in GPS when no configured device is
       available on this platform */
    LogString("Falling back to built-in GPS");

    DeviceConfig config;
    config.Clear();
    config.port_type = DeviceConfig::PortType::INTERNAL;

    DeviceDescriptor &device = devices[0];
    devInitOne(device, config);
#endif
  }
}

void
devRestart(MultipleDevices &devices, const SystemSettings &settings)
{
  LogString("RestartCommPorts");

  devices.Close();

  devStartup(devices, settings);

  /* this OperationEnvironment instance must be persistent, because
     DeviceDescriptor::Open() is asynchronous */
  static PopupOperationEnvironment env;
  devices.Open(env);
}
