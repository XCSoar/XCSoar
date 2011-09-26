/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "Device/List.hpp"
#include "Device/Descriptor.hpp"
#include "LogFile.hpp"
#include "Profile/Profile.hpp"
#include "Profile/DeviceConfig.hpp"
#include "Operation.hpp"

#include <assert.h>

static bool
devInitOne(DeviceDescriptor &device, DeviceDescriptor *nmeaout)
{
  PopupOperationEnvironment env;

  if (!device.Open(env))
    return false;

  if (nmeaout == NULL && device.IsNMEAOut())
    nmeaout = &device;

  return true;
}

static void
SetPipeTo(DeviceDescriptor &out)
{
  for (unsigned i = 0; i < NUMDEV; ++i) {
    DeviceDescriptor &device = DeviceList[i];

    device.SetPipeTo(&device == &out ? NULL : &out);
  }
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
  switch (a.port_type) {
  case DeviceConfig::PortType::SERIAL:
    return b.port_type == DeviceConfig::PortType::SERIAL &&
      a.path.equals(b.path);

  case DeviceConfig::PortType::RFCOMM:
    return b.port_type == DeviceConfig::PortType::RFCOMM &&
      a.bluetooth_mac.equals(b.bluetooth_mac);

  case DeviceConfig::PortType::IOIOUART:
    return (b.port_type == DeviceConfig::PortType::IOIOUART) &&
      a.ioio_uart_id == b.ioio_uart_id;

  case DeviceConfig::PortType::DISABLED:
  case DeviceConfig::PortType::AUTO:
  case DeviceConfig::PortType::INTERNAL:
  case DeviceConfig::PortType::TCP_LISTENER:
    break;
  }

  return a.port_type == b.port_type;
}

void
devStartup()
{
  LogStartUp(_T("Register serial devices"));

  DeviceDescriptor *pDevNmeaOut = NULL;

  Profile::Get(szProfileIgnoreNMEAChecksum, NMEAParser::ignore_checksum);

  bool none_available = true;
  for (unsigned i = 0; i < NUMDEV; ++i) {
    DeviceList[i].SetIndex(i);

    DeviceConfig &config = DeviceList[i].SetConfig();
    Profile::GetDeviceConfig(i, config);
    if (!config.IsAvailable()) {
      config.Clear();
      continue;
    }

    none_available = false;

    bool overlap = false;
    for (unsigned j = 0; j < i; ++j)
      if (DeviceConfigOverlaps(config, DeviceList[j].GetConfig()))
        overlap = true;

    if (overlap) {
      config.Clear();
      continue;
    }

    devInitOne(DeviceList[i], pDevNmeaOut);
  }

  if (none_available) {
#ifdef ANDROID
    /* fall back to built-in GPS when no configured device is
       available on this platform */
    LogStartUp(_T("Falling back to built-in GPS"));

    DeviceConfig &config = DeviceList[0].SetConfig();
    config.Clear();
    config.port_type = DeviceConfig::PortType::INTERNAL;
    devInitOne(DeviceList[0], pDevNmeaOut);
#endif
  }

  if (pDevNmeaOut != NULL)
    SetPipeTo(*pDevNmeaOut);
}

bool
HaveCondorDevice()
{
  for (unsigned i = 0; i < NUMDEV; ++i)
    if (DeviceList[i].IsCondor())
      return true;

  return false;
}

void
VarioWriteNMEA(const TCHAR *text)
{
  for (int i = 0; i < NUMDEV; i++)
    if (DeviceList[i].IsVega())
      DeviceList[i].WriteNMEA(text);
}

DeviceDescriptor *
devVarioFindVega(void)
{
  for (int i = 0; i < NUMDEV; i++)
    if (DeviceList[i].IsVega())
      return &DeviceList[i];

  return NULL;
}

void
devShutdown()
{
  int i;

  // Stop COM devices
  LogStartUp(_T("Stop COM devices"));

  for (i = 0; i < NUMDEV; i++) {
    DeviceList[i].Close();
  }
}

void
devRestart()
{
  LogStartUp(_T("RestartCommPorts"));

  devShutdown();

  devStartup();
}
