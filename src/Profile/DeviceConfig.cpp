/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Profile/DeviceConfig.hpp"
#include "Profile/Profile.hpp"
#include "Asset.hpp"
#include "Language/Language.hpp"
#include "Util/Macros.hpp"
#include "Interface.hpp"

#include <stdio.h>

static const TCHAR *const port_type_strings[] = {
  _T("disabled"),
  _T("serial"),
  _T("rfcomm"),
  _T("rfcomm_server"),
  _T("ioio_uart"),
  _T("droidsoar_v2"),
  _T("nunchuck"),
  _T("i2c_baro"),
  _T("auto"),
  _T("internal"),
  _T("tcp_listener"),
  _T("udp_listener"),
  _T("pty"),
  NULL
};

bool
DeviceConfig::IsAvailable() const
{
  switch (port_type) {
  case PortType::DISABLED:
    return false;

  case PortType::SERIAL:
    return true;

  case PortType::RFCOMM:
  case PortType::RFCOMM_SERVER:
    return IsAndroid();

  case PortType::IOIOUART:
  case PortType::DROIDSOAR_V2:
  case PortType::NUNCHUCK:
  case PortType::I2CPRESSURESENSOR:
    return IsAndroid() && HasIOIOLib();

  case PortType::AUTO:
    return IsWindowsCE();

  case PortType::INTERNAL:
    return IsAndroid();

  case PortType::TCP_LISTENER:
  case PortType::UDP_LISTENER:
    return true;

  case PortType::PTY:
#if defined(HAVE_POSIX) && !defined(ANDROID)
    return true;
#else
    return false;
#endif
  }

  /* unreachable */
  return false;
}

bool
DeviceConfig::ShouldReopenOnTimeout() const
{
  switch (port_type) {
  case PortType::DISABLED:
    return false;

  case PortType::SERIAL:
  case PortType::AUTO:
    /* auto-reopen on Windows CE due to its quirks, but not Altair,
       because Altair ports are known to be "kind of sane" (no flaky
       Bluetooth drivers, because there's no Bluetooth) */
    return IsWindowsCE() && !IsAltair();

  case PortType::RFCOMM:
  case PortType::RFCOMM_SERVER:
  case PortType::IOIOUART:
  case PortType::DROIDSOAR_V2:
  case PortType::NUNCHUCK:
  case PortType::I2CPRESSURESENSOR:
    /* errors on these are detected automatically by the driver */
    return false;

  case PortType::INTERNAL:
    /* reopening the Android internal GPS doesn't help */
    return false;

  case PortType::TCP_LISTENER:
  case PortType::UDP_LISTENER:
    /* this is a server, and if no data gets received, this can just
       mean that nobody connected to it, but reopening it periodically
       doesn't help */
    return false;

  case PortType::PTY:
    return false;
  }

  /* unreachable */
  assert(false);
  return false;
}

const TCHAR *
DeviceConfig::GetPortName(TCHAR *buffer, size_t max_size) const
{
  switch (port_type) {
  case PortType::DISABLED:
    return _("Disabled");

  case PortType::SERIAL:
    return path.c_str();

  case PortType::RFCOMM:
    _sntprintf(buffer, max_size, _T("Bluetooth %s"),
               bluetooth_mac.c_str());
    return buffer;

  case PortType::RFCOMM_SERVER:
    return _("Bluetooth server");

  case PortType::IOIOUART:
    _sntprintf(buffer, max_size, _T("IOIO UART %d"),
               ioio_uart_id);
    return buffer;

  case PortType::DROIDSOAR_V2:
    return _T("DroidSoar V2");

  case PortType::NUNCHUCK:
    return _T("Nunchuck");

  case PortType::I2CPRESSURESENSOR:
    return _T("IOIO i2c pressure sensor");

  case PortType::AUTO:
    return _("GPS Intermediate Driver");

  case PortType::INTERNAL:
    return _("Built-in GPS & sensors");

  case PortType::TCP_LISTENER:
    _sntprintf(buffer, max_size, _T("TCP port %d"), tcp_port);
    return buffer;

  case PortType::UDP_LISTENER:
    _sntprintf(buffer, max_size, _T("UDP port %d"), tcp_port);
    return buffer;

  case PortType::PTY:
    _sntprintf(buffer, max_size, _T("Pseudo-terminal %s"), path.c_str());
    return buffer;
  }

  /* unreachable */
  assert(false);
  return _T("Disabled");
}

static const TCHAR *
MakeDeviceSettingName(TCHAR *buffer, const TCHAR *prefix, unsigned n,
                      const TCHAR *suffix)
{
  _tcscpy(buffer, prefix);

  if (n > 0)
    _stprintf(buffer + _tcslen(buffer), _T("%u"), n + 1);

  _tcscat(buffer, suffix);

  return buffer;
}

static bool
StringToPortType(const TCHAR *value, DeviceConfig::PortType &type)
{
  for (auto i = port_type_strings; *i != NULL; ++i) {
    if (StringIsEqual(value, *i)) {
      type = (DeviceConfig::PortType)std::distance(port_type_strings, i);
      return true;
    }
  }

  return false;
}

static bool
ReadPortType(unsigned n, DeviceConfig::PortType &type)
{
  TCHAR name[64];

  MakeDeviceSettingName(name, _T("Port"), n, _T("Type"));

  const TCHAR *value = Profile::Get(name);
  return value != NULL && StringToPortType(value, type);
}

static bool
LoadPath(DeviceConfig &config, unsigned n)
{
  TCHAR buffer[64];
  MakeDeviceSettingName(buffer, _T("Port"), n, _T("Path"));
  return Profile::Get(buffer, config.path);
}

static bool
LoadPortIndex(DeviceConfig &config, unsigned n)
{
  TCHAR buffer[64];
  MakeDeviceSettingName(buffer, _T("Port"), n, _T("Index"));

  unsigned index;
  if (!Profile::Get(buffer, index))
    return false;

  /* adjust the number, compatibility quirk for XCSoar 5 */
  if (index < 10)
    ++index;
  else if (index == 10)
    index = 0;

  _stprintf(buffer, _T("COM%u:"), index);
  config.path = buffer;
  return true;
}

void
Profile::GetDeviceConfig(unsigned n, DeviceConfig &config)
{
  TCHAR buffer[64];

  bool have_port_type = ReadPortType(n, config.port_type);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("BluetoothMAC"));
  Get(buffer, config.bluetooth_mac);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("IOIOUartID"));
  Get(buffer, config.ioio_uart_id);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("TCPPort"));
  if (!Get(buffer, config.tcp_port))
    config.tcp_port = 4353;

  config.path.clear();
  if ((!have_port_type ||
       config.port_type == DeviceConfig::PortType::SERIAL) &&
      !LoadPath(config, n) && LoadPortIndex(config, n))
    config.port_type = DeviceConfig::PortType::SERIAL;

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("BaudRate"));
  if (!Get(buffer, config.baud_rate)) {
    /* XCSoar before 6.2 used to store a "speed index", not the real
       baud rate - try to import the old settings */

    static constexpr unsigned speed_index_table[] = {
      1200,
      2400,
      4800,
      9600,
      19200,
      38400,
      57600,
      115200
    };

    MakeDeviceSettingName(buffer, _T("Speed"), n, _T("Index"));
    unsigned speed_index;
    if (Get(buffer, speed_index) &&
        speed_index < ARRAY_SIZE(speed_index_table))
      config.baud_rate = speed_index_table[speed_index];
  }

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("BulkBaudRate"));
  if (!Get(buffer, config.bulk_baud_rate))
    config.bulk_baud_rate = 0;

  _tcscpy(buffer, _T("DeviceA"));
  buffer[_tcslen(buffer) - 1] += n;
  Get(buffer, config.driver_name);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("SyncFromDevice"));
  Get(buffer, config.sync_from_device);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("SyncToDevice"));
  Get(buffer, config.sync_to_device);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("K6Bt"));
  Get(buffer, config.k6bt);

#ifndef NDEBUG
  MakeDeviceSettingName(buffer, _T("Port"), n, _T("DumpPort"));
  Get(buffer, config.dump_port);
#endif

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("IgnoreChecksum"));
  if (!Get(buffer, config.ignore_checksum))
    Get(ProfileKeys::IgnoreNMEAChecksum, config.ignore_checksum);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("I2C_Bus"));
  Get(buffer, config.i2c_bus);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("I2C_Addr"));
  Get(buffer, config.i2c_addr);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("PressureUse"));
  GetEnum(buffer, config.press_use);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("PitotOffset"));
  Get(buffer, config.pitot_offset);
}

static const TCHAR *
PortTypeToString(DeviceConfig::PortType type)
{
  const unsigned i = (unsigned)type;
  return i < ARRAY_SIZE(port_type_strings)
    ? port_type_strings[i]
    : NULL;
}

static void
WritePortType(unsigned n, DeviceConfig::PortType type)
{
  const TCHAR *value = PortTypeToString(type);
  if (value == NULL)
    return;

  TCHAR name[64];

  MakeDeviceSettingName(name, _T("Port"), n, _T("Type"));
  Profile::Set(name, value);
}

void
Profile::SetDeviceConfig(unsigned n, const DeviceConfig &config)
{
  TCHAR buffer[64];

  WritePortType(n, config.port_type);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("BluetoothMAC"));
  Set(buffer, config.bluetooth_mac);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("IOIOUartID"));
  Set(buffer, config.ioio_uart_id);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("Path"));
  Set(buffer, config.path);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("BaudRate"));
  Set(buffer, config.baud_rate);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("BulkBaudRate"));
  Set(buffer, config.bulk_baud_rate);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("TCPPort"));
  Set(buffer, config.tcp_port);

  _tcscpy(buffer, _T("DeviceA"));
  buffer[_tcslen(buffer) - 1] += n;
  Set(buffer, config.driver_name);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("SyncFromDevice"));
  Set(buffer, config.sync_from_device);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("SyncToDevice"));
  Set(buffer, config.sync_to_device);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("K6Bt"));
  Set(buffer, config.k6bt);

#ifndef NDEBUG
  MakeDeviceSettingName(buffer, _T("Port"), n, _T("DumpPort"));
  Set(buffer, config.dump_port);
#endif

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("IgnoreChecksum"));
  Set(buffer, config.ignore_checksum);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("I2C_Bus"));
  Set(buffer, config.i2c_bus);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("I2C_Addr"));
  Set(buffer, config.i2c_addr);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("PressureUse"));
  SetEnum(buffer, config.press_use);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("PitotOffset"));
  if (config.port_type == DeviceConfig::PortType::DROIDSOAR_V2 ||
      config.port_type == DeviceConfig::PortType::I2CPRESSURESENSOR) {
    // Has a new offset been determined ?
    if (XCSoarInterface::Basic().pitot_offset_available) {
      Set(buffer, XCSoarInterface::Basic().pitot_offset);
    } else {
      Set(buffer, config.pitot_offset);
    }
  } else {
    Set(buffer, fixed(0.0));	// invalid value
  }
}
