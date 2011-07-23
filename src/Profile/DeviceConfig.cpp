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

#include "Profile/DeviceConfig.hpp"
#include "Profile/Profile.hpp"
#include "Asset.hpp"
#include "Language/Language.hpp"

#include <stdio.h>

bool
DeviceConfig::IsAvailable() const
{
  switch (port_type) {
  case DISABLED:
    return false;

  case SERIAL:
    return !is_android();

  case RFCOMM:
    return is_android();

  case IOIOUART:
    return is_android() && is_ioiolib();

  case AUTO:
    return is_windows_ce();

  case INTERNAL:
    return is_android();

  case TCP_LISTENER:
    return true;
  }

  /* unreachable */
  return false;
}

const TCHAR *
DeviceConfig::GetPortName(TCHAR *buffer, size_t max_size) const
{
  switch (port_type) {
  case DeviceConfig::DISABLED:
    return _("Disabled");

  case DeviceConfig::SERIAL:
    return path.c_str();

  case DeviceConfig::RFCOMM:
    _sntprintf(buffer, max_size, _T("Bluetooth %s"),
               bluetooth_mac.c_str());
    return buffer;

  case DeviceConfig::IOIOUART:
    _sntprintf(buffer, max_size, _T("IOIO UArt %d"),
               ioio_uart_id);
    return buffer;

  case DeviceConfig::AUTO:
    return _("GPS Intermediate Driver");

  case DeviceConfig::INTERNAL:
    return _("Built-in GPS");

  case DeviceConfig::TCP_LISTENER:
    return _T("TCP port 4353");
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

static enum DeviceConfig::port_type
StringToPortType(const TCHAR *value)
{
  if (_tcscmp(value, _T("disabled")) == 0)
    return DeviceConfig::DISABLED;

  if (_tcscmp(value, _T("serial")) == 0)
    return DeviceConfig::SERIAL;

  if (_tcscmp(value, _T("rfcomm")) == 0)
    return DeviceConfig::RFCOMM;

  if (_tcscmp(value, _T("ioio_uart")) == 0)
    return DeviceConfig::IOIOUART;

  if (_tcscmp(value, _T("auto")) == 0)
    return DeviceConfig::AUTO;

  if (_tcscmp(value, _T("internal")) == 0)
    return DeviceConfig::INTERNAL;

  if (_tcscmp(value, _T("tcp_listener")) == 0)
    return DeviceConfig::TCP_LISTENER;

  if (is_android())
    return DeviceConfig::INTERNAL;

  return DeviceConfig::SERIAL;
}

static enum DeviceConfig::port_type
ReadPortType(unsigned n)
{
  TCHAR name[64], value[64];

  MakeDeviceSettingName(name, _T("Port"), n, _T("Type"));
  if (!Profile::Get(name, value, sizeof(value) / sizeof(value[0])))
    return n == 0
      ? (is_android()
         ? DeviceConfig::INTERNAL
         : DeviceConfig::SERIAL)
      : DeviceConfig::DISABLED;

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

  config.path.clear();
  if (config.port_type == DeviceConfig::SERIAL &&
      !LoadPath(config, n) && !LoadPortIndex(config, n)) {
    if (is_altair() && n == 0)
      config.path = _T("COM3:");
    else if (is_altair() && n == 2)
      config.path = _T("COM2:");
  }

  MakeDeviceSettingName(buffer, _T("Port"), n, _T("BaudRate"));
  if (!Get(buffer, config.baud_rate)) {
    /* XCSoar before 6.2 used to store a "speed index", not the real
       baud rate - try to import the old settings */

    static const unsigned speed_index_table[] = {
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
        speed_index < sizeof(speed_index_table) / sizeof(speed_index_table[0]))
      config.baud_rate = speed_index_table[speed_index];
    else if (is_altair())
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
    if (is_altair() && n == 0)
      config.driver_name = _T("Altair RU");
    else if (is_altair() && n == 1)
      config.driver_name = _T("Vega");
    else if (is_altair() && n == 2)
      config.driver_name = _T("NmeaOut");
    else
      config.driver_name.clear();
  }
}

static const TCHAR *
PortTypeToString(enum DeviceConfig::port_type type)
{
  switch (type) {
  case DeviceConfig::DISABLED:
    return _T("disabled");

  case DeviceConfig::SERIAL:
    return _T("serial");

  case DeviceConfig::RFCOMM:
    return _T("rfcomm");

  case DeviceConfig::IOIOUART:
    return _T("ioio_uart");

  case DeviceConfig::AUTO:
    return _T("auto");

  case DeviceConfig::INTERNAL:
    return _T("internal");

  case DeviceConfig::TCP_LISTENER:
    return _T("tcp_listener");
  }

  return NULL;
}

static bool
WritePortType(unsigned n, enum DeviceConfig::port_type type)
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

  _tcscpy(buffer, _T("DeviceA"));
  buffer[_tcslen(buffer) - 1] += n;
  Set(buffer, config.driver_name);
}
