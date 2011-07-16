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

#include "Device/device.hpp"
#include "Device/Internal.hpp"
#include "Device/Driver.hpp"
#include "Device/Register.hpp"
#include "Device/List.hpp"
#include "Device/Descriptor.hpp"
#include "Device/Parser.hpp"
#include "Device/Port.hpp"
#include "Device/NullPort.hpp"
#include "LogFile.hpp"
#include "DeviceBlackboard.hpp"
#include "Message.hpp"
#include "Language/Language.hpp"
#include "Asset.hpp"
#include "../Simulator.hpp"
#include "Profile/Profile.hpp"
#include "Profile/DeviceConfig.hpp"
#include "Device/TCPPort.hpp"

#ifdef _WIN32_WCE
#include "Config/Registry.hpp"
#endif

#ifdef ANDROID
#include "Android/InternalGPS.hpp"
#include "Android/Main.hpp"
#include "Java/Object.hpp"
#include "Java/Global.hpp"
#include "Device/AndroidBluetoothPort.hpp"
#elif defined(HAVE_POSIX)
#include "Device/TTYPort.hpp"
#else
#include "Device/SerialPort.hpp"
#endif

#include <assert.h>
#include <windef.h> /* for MAX_PATH */

/**
 * Attempt to detect the GPS device.
 *
 * See http://msdn.microsoft.com/en-us/library/bb202042.aspx
 */
static bool
detect_gps(TCHAR *path, size_t path_max_size)
{
#ifdef _WIN32_WCE
  static const TCHAR *const gps_idm_key =
    _T("System\\CurrentControlSet\\GPS Intermediate Driver\\Multiplexer");
  static const TCHAR *const gps_idm_value = _T("DriverInterface");

  RegistryKey key(HKEY_LOCAL_MACHINE, gps_idm_key, true);
  return !key.error() &&
    key.get_value(gps_idm_value, path, path_max_size);
#else
  return false;
#endif
}

static Port *
OpenPort(const DeviceConfig &config, Port::Handler &handler)
{
  if (is_simulator())
    return new NullPort(handler);

  const TCHAR *path = NULL;
  TCHAR buffer[MAX_PATH];

  switch (config.port_type) {
  case DeviceConfig::DISABLED:
    return NULL;

  case DeviceConfig::SERIAL:
    if (config.path.empty())
      return NULL;

    path = config.path.c_str();
    break;

  case DeviceConfig::RFCOMM:
#ifdef ANDROID
    if (config.bluetooth_mac.empty()) {
      LogStartUp(_T("No Bluetooth MAC configured"));
      return NULL;
    }

    {
      AndroidBluetoothPort *port =
        new AndroidBluetoothPort(config.bluetooth_mac, handler);
      if (!port->Open()) {
        delete port;
        return NULL;
      }

      return port;
    }
#else
    LogStartUp(_T("Bluetooth not available on this platform"));
    return NULL;
#endif

  case DeviceConfig::AUTO:
    if (!detect_gps(buffer, sizeof(buffer))) {
      LogStartUp(_T("no GPS detected"));
      return NULL;
    }

    LogStartUp(_T("GPS detected: %s"), buffer);

    path = buffer;
    break;

  case DeviceConfig::INTERNAL:
    break;

  case DeviceConfig::TCP_LISTENER: {
    TCPPort *port = new TCPPort(4353, handler);
    if (!port->Open()) {
      delete port;
      return NULL;
    }

    return port;
  }
  }

#ifdef ANDROID
  return NULL;
#else
  if (path == NULL)
    return NULL;

#ifdef HAVE_POSIX
  TTYPort *Com = new TTYPort(path, config.baud_rate, handler);
#else
  SerialPort *Com = new SerialPort(path, config.baud_rate, handler);
#endif
  if (!Com->Open()) {
    delete Com;
    return NULL;
  }

  return Com;
#endif
}

/**
 * The configured port failed to open; display an error message.
 */
static void
PortOpenError(const DeviceConfig &config)
{
  TCHAR name_buffer[64];
  const TCHAR *name = name_buffer;
  switch (config.port_type) {
  case DeviceConfig::DISABLED:
    name = _("Disabled");
    break;

  case DeviceConfig::SERIAL:
    name = config.path.c_str();
    break;

  case DeviceConfig::RFCOMM:
    _sntprintf(name_buffer, 64, _T("Bluetooth %s"),
               config.bluetooth_mac.c_str());
    break;

  case DeviceConfig::AUTO:
    name = _("GPS Intermediate Driver");
    break;

  case DeviceConfig::INTERNAL:
    name = _("Built-in GPS");
    break;

  case DeviceConfig::TCP_LISTENER:
    name = _T("TCP port 4353");
    break;
  }

  TCHAR msg[256];
  _sntprintf(msg, 256, _("Unable to open port %s"), name);
  Message::AddMessage(msg);
}

static bool
devInitOne(DeviceDescriptor &device, const DeviceConfig &config,
           DeviceDescriptor *&nmeaout)
{
  if (config.port_type == DeviceConfig::INTERNAL) {
#ifdef ANDROID
    if (is_simulator())
      return true;

    device.internal_gps = InternalGPS::create(Java::GetEnv(), context,
                                              device.GetIndex());
    return device.internal_gps != NULL;
#else
    return false;
#endif
  }

  const struct DeviceRegister *Driver = FindDriverByName(config.driver_name);
  if (Driver == NULL)
    return false;

  Port *Com = OpenPort(config, device);
  if (Com == NULL) {
    PortOpenError(config);
    return false;
  }

  if (!device.Open(Com, Driver)) {
    delete Com;
    return false;
  }

  if (nmeaout == NULL && Driver->IsNMEAOut())
    nmeaout = &device;

  return true;
}

static void
SetPipeTo(DeviceDescriptor &out)
{
  for (unsigned i = 0; i < NUMDEV; ++i) {
    DeviceDescriptor &device = DeviceList[i];

    device.pDevPipeTo = &device == &out ? NULL : &out;
  }
}

/**
 * Checks if the specified DeviceConfig is available on this platform.
 */
gcc_pure
static bool
DeviceConfigAvailable(const DeviceConfig &config)
{
  switch (config.port_type) {
  case DeviceConfig::DISABLED:
    return false;

  case DeviceConfig::SERIAL:
    return !is_android();

  case DeviceConfig::RFCOMM:
    return is_android();

  case DeviceConfig::AUTO:
    return is_windows_ce();

  case DeviceConfig::INTERNAL:
    return is_android();

  case DeviceConfig::TCP_LISTENER:
    return true;
  }

  /* unreachable */
  return false;
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
  case DeviceConfig::SERIAL:
    return b.port_type == DeviceConfig::SERIAL &&
      a.path.equals(b.path);

  case DeviceConfig::RFCOMM:
    return b.port_type == DeviceConfig::RFCOMM &&
      a.bluetooth_mac.equals(b.bluetooth_mac);

  case DeviceConfig::DISABLED:
  case DeviceConfig::AUTO:
  case DeviceConfig::INTERNAL:
  case DeviceConfig::TCP_LISTENER:
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

  DeviceConfig config[NUMDEV];
  bool none_available = true;
  for (unsigned i = 0; i < NUMDEV; ++i) {
    DeviceList[i].SetIndex(i);

    Profile::GetDeviceConfig(i, config[i]);

    if (!DeviceConfigAvailable(config[i]))
      continue;

    none_available = false;

    bool overlap = false;
    for (unsigned j = 0; j < i; ++j)
      if (DeviceConfigOverlaps(config[i], config[j]))
        overlap = true;

    if (!overlap)
      devInitOne(DeviceList[i], config[i], pDevNmeaOut);
  }

  if (none_available) {
#ifdef ANDROID
    /* fall back to built-in GPS when no configured device is
       available on this platform */
    LogStartUp(_T("Falling back to built-in GPS"));

    config[0].port_type = DeviceConfig::INTERNAL;
    devInitOne(DeviceList[0], config[0], pDevNmeaOut);
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

#ifdef _UNICODE
static void
PortWriteNMEA(Port *port, const TCHAR *line)
{
  assert(port != NULL);
  assert(line != NULL);

  char buffer[_tcslen(line) * 4 + 1];
  if (::WideCharToMultiByte(CP_ACP, 0, line, -1, buffer, sizeof(buffer),
                            NULL, NULL) <= 0)
    return;

  PortWriteNMEA(port, buffer);
}
#endif

void
devWriteNMEAString(DeviceDescriptor &d, const TCHAR *text)
{
  if (d.Com == NULL)
    return;

  PortWriteNMEA(d.Com, text);
}

void
VarioWriteNMEA(const TCHAR *text)
{
  for (int i = 0; i < NUMDEV; i++)
    if (DeviceList[i].IsVega())
      if (DeviceList[i].Com)
        PortWriteNMEA(DeviceList[i].Com, text);
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
