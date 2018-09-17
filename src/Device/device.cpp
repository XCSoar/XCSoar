/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

// 20070413:sgi add NmeaOut support, allow nmea chaining an double port platforms

#include "device.hpp"
#include "Features.hpp"
#include "Device/MultipleDevices.hpp"
#include "Device/Descriptor.hpp"
#include "Components.hpp"
#include "LogFile.hpp"
#include "Interface.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "Util/Algorithm.hpp"

static void
devInitOne(DeviceDescriptor &device, const DeviceConfig &config)
{
  device.SetConfig(config);

  /* this OperationEnvironment instance must be persistent, because
     DeviceDescriptor::Open() is asynchronous */
  static PopupOperationEnvironment env;

  device.ResetFailureCounter();
  device.Open(env);
}

/**
 * Checks if the two configurations overlap, i.e. they request access
 * to an exclusive resource, like the same physical COM port.  If this
 * is detected, then the second device will be disabled.
 */
gcc_pure
static bool
DeviceConfigOverlaps(const DeviceConfig &a, const DeviceConfig &b)
{
  if (a.port_type != b.port_type)
    return false;

  switch (a.port_type) {
  case DeviceConfig::PortType::SERIAL:
  case DeviceConfig::PortType::PTY:
    return a.path.equals(b.path);

  case DeviceConfig::PortType::RFCOMM:
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
gcc_pure
static bool
DeviceConfigOverlaps(const DeviceConfig &config, I begin, I end)
{
  return ExistsIf(begin, end,
                  [&config](const DeviceDescriptor *d) {
                    return DeviceConfigOverlaps(config, d->GetConfig());
                  });
}

void
devStartup()
{
  LogFormat("Register serial devices");

  const SystemSettings &settings = CommonInterface::GetSystemSettings();

  bool none_available = true;
  for (unsigned i = 0; i < NUMDEV; ++i) {
    DeviceDescriptor &device = (*devices)[i];
    const DeviceConfig &config = settings.devices[i];
    if (!config.IsAvailable()) {
      device.ClearConfig();
      continue;
    }

    none_available = false;

    if (DeviceConfigOverlaps(config, devices->begin(), devices->begin() + i)) {
      device.ClearConfig();
      continue;
    }

    devInitOne(device, config);
  }

  if (none_available) {
#ifdef HAVE_INTERNAL_GPS
    /* fall back to built-in GPS when no configured device is
       available on this platform */
    LogFormat("Falling back to built-in GPS");

    DeviceConfig config;
    config.Clear();
    config.port_type = DeviceConfig::PortType::INTERNAL;

    DeviceDescriptor &device = (*devices)[0];
    devInitOne(device, config);
#endif
  }
}

void
VarioWriteNMEA(const TCHAR *text, OperationEnvironment &env)
{
  for (DeviceDescriptor *i : *devices)
    if (i->IsVega())
      i->WriteNMEA(text, env);
}

DeviceDescriptor *
devVarioFindVega()
{
  for (DeviceDescriptor *i : *devices)
    if (i->IsVega())
      return i;

  return nullptr;
}

void
devShutdown()
{
  if (devices == nullptr)
    return;

  // Stop COM devices
  LogFormat("Stop COM devices");

  for (DeviceDescriptor *i : *devices)
    i->Close();
}

void
devRestart()
{
  LogFormat("RestartCommPorts");

  devShutdown();

  devStartup();
}
