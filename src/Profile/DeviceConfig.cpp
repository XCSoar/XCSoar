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

#include <stdio.h>

bool
DeviceConfig::IsAvailable() const
{
  switch (port_type) {
  case PortType::DISABLED:
    return false;

  case PortType::SERIAL:
    return true;

  case PortType::RFCOMM:
    return IsAndroid();

  case PortType::IOIOUART:
    return IsAndroid() && HasIOIOLib();

  case PortType::AUTO:
    return IsWindowsCE();

  case PortType::INTERNAL:
    return IsAndroid();

  case PortType::TCP_LISTENER:
    return true;
  }

  /* unreachable */
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

  case PortType::IOIOUART:
    _sntprintf(buffer, max_size, _T("IOIO UART %d"),
               ioio_uart_id);
    return buffer;

  case PortType::AUTO:
    return _("GPS Intermediate Driver");

  case PortType::INTERNAL:
    return _("Built-in GPS");

  case PortType::TCP_LISTENER:
    _sntprintf(buffer, max_size, _T("TCP port %d"), tcp_port);
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

static DeviceConfig::PortType
StringToPortType(const TCHAR *value)
{
  if (StringIsEqual(value, _T("disabled")))
    return DeviceConfig::PortType::DISABLED;

  if (StringIsEqual(value, _T("serial")))
    return DeviceConfig::PortType::SERIAL;

  if (StringIsEqual(value, _T("rfcomm")))
    return DeviceConfig::PortType::RFCOMM;

  if (StringIsEqual(value, _T("ioio_uart")))
    return DeviceConfig::PortType::IOIOUART;

  if (StringIsEqual(value, _T("auto")))
    return DeviceConfig::PortType::AUTO;

  if (StringIsEqual(value, _T("internal")))
    return DeviceConfig::PortType::INTERNAL;

  if (StringIsEqual(value, _T("tcp_listener")))
    return DeviceConfig::PortType::TCP_LISTENER;

  if (IsAndroid())
    return DeviceConfig::PortType::INTERNAL;

  return DeviceConfig::PortType::SERIAL;
}

static DeviceConfig::PortType
ReadPortType(unsigned n)
{
  TCHAR name[64], value[64];

  MakeDeviceSettingName(name, _T("Port"), n, _T("Type"));
  if (!Profile::Get(name, value, ARRAY_SIZE(value)))
    return n == 0
      ? (IsAndroid()
         ? DeviceConfig::PortType::INTERNAL
         : DeviceConfig::PortType::SERIAL)
      : DeviceConfig::PortType::DISABLED;

  return StringToPortType(value);
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

  config.port_type = ReadPortType(n);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("BluetoothMAC"));
  Get(buffer, config.bluetooth_mac);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("IOIOUartID"));
  Get(buffer, config.ioio_uart_id);

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("TCPPort"));
  if (!Get(buffer, config.tcp_port))
    config.tcp_port = 4353;

  config.path.clear();
  if (config.port_type == DeviceConfig::PortType::SERIAL &&
      !LoadPath(config, n) && !LoadPortIndex(config, n)) {
    if (IsAltair() && n == 0)
      config.path = _T("COM3:");
    else if (IsAltair() && n == 2)
      config.path = _T("COM2:");
    else
      config.port_type = DeviceConfig::PortType::DISABLED;
  }

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("BaudRate"));
  if (!Get(buffer, config.baud_rate)) {
    /* XCSoar before 6.2 used to store a "speed index", not the real
       baud rate - try to import the old settings */

    static gcc_constexpr_data unsigned speed_index_table[] = {
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
    else if (IsAltair())
      config.baud_rate = 38400;
    else
      config.baud_rate = 4800;
  }

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("BulkBaudRate"));
  if (!Get(buffer, config.bulk_baud_rate))
    config.bulk_baud_rate = 0;

  _tcscpy(buffer, _T("DeviceA"));
  buffer[_tcslen(buffer) - 1] += n;
  if (!Get(buffer, config.driver_name)) {
    if (IsAltair() && n == 0)
      config.driver_name = _T("Altair RU");
    else if (IsAltair() && n == 1)
      config.driver_name = _T("Vega");
    else if (IsAltair() && n == 2)
      config.driver_name = _T("NmeaOut");
    else
      config.driver_name.clear();
  }

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("SyncFromDevice"));
  if (!Get(buffer, config.sync_from_device))
    config.sync_from_device = true;

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("SyncToDevice"));
  if (!Get(buffer, config.sync_to_device))
    config.sync_to_device = true;
}

static const TCHAR *
PortTypeToString(DeviceConfig::PortType type)
{
  switch (type) {
  case DeviceConfig::PortType::DISABLED:
    return _T("disabled");

  case DeviceConfig::PortType::SERIAL:
    return _T("serial");

  case DeviceConfig::PortType::RFCOMM:
    return _T("rfcomm");

  case DeviceConfig::PortType::IOIOUART:
    return _T("ioio_uart");

  case DeviceConfig::PortType::AUTO:
    return _T("auto");

  case DeviceConfig::PortType::INTERNAL:
    return _T("internal");

  case DeviceConfig::PortType::TCP_LISTENER:
    return _T("tcp_listener");
  }

  return NULL;
}

static bool
WritePortType(unsigned n, DeviceConfig::PortType type)
{
  const TCHAR *value = PortTypeToString(type);
  if (value == NULL)
    return false;

  TCHAR name[64];

  MakeDeviceSettingName(name, _T("Port"), n, _T("Type"));
  return Profile::Set(name, value);
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
}
