// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SystemSettings.hpp"
#include "Asset.hpp"
#include "Device/Features.hpp"
#ifdef KOBO
#include "Kobo/Model.hpp"
#endif

void
SystemSettings::SetDefaults()
{
  for (unsigned i = 0; i < devices.size(); ++i)
    devices[i].Clear();

  if (IsAndroid() || IsApple()) {
    devices[INTERNAL_DEVICE_SLOT].port_type = DeviceConfig::PortType::INTERNAL;
  } else {
    devices[0].port_type = DeviceConfig::PortType::SERIAL;
#ifdef _WIN32
    devices[0].path = "COM1:";
#else
#ifdef KOBO
    if (IsKoboMediaTek()) {
      devices[0].path = "/dev/ttyS0";
      devices[0].baud_rate = 9600;
    } else
#endif
    {
      devices[0].path = "/dev/tty0";
      devices[0].baud_rate = 4800;
    }
#endif
    devices[0].driver_name = "Generic";
  }
}
