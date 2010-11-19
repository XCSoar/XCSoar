/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include <stdio.h>

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
  if (_tcscmp(value, _T("serial")) == 0)
    return DeviceConfig::SERIAL;

  if (_tcscmp(value, _T("auto")) == 0)
    return DeviceConfig::AUTO;

  return DeviceConfig::SERIAL;
}

static enum DeviceConfig::port_type
ReadPortType(unsigned n)
{
  TCHAR name[64], value[64];

  MakeDeviceSettingName(name, CONF("Port"), n, _T("Type"));
  if (!Profile::Get(name, value, sizeof(value) / sizeof(value[0])))
    return DeviceConfig::SERIAL;

  return StringToPortType(value);
}

void
Profile::GetDeviceConfig(unsigned n, DeviceConfig &config)
{
  TCHAR buffer[64];
  unsigned Temp = 0;

  config.port_type = ReadPortType(n);

  MakeDeviceSettingName(buffer, CONF("Port"), n, _T("Index"));
  if (Get(buffer, Temp))
    config.port_index = Temp;
  else if (is_altair() && n == 0)
    config.port_index = 2;
  else
    config.port_index = 0;

  MakeDeviceSettingName(buffer, CONF("Speed"), n, _T("Index"));
  if (Get(buffer, Temp))
    config.speed_index = Temp;
  else if (is_altair())
    config.speed_index = 5;
  else
    config.speed_index = 2;

  _tcscpy(buffer, CONF("DeviceA"));
  buffer[_tcslen(buffer) - 1] += n;
  if (!Get(buffer, config.driver_name,
           sizeof(config.driver_name) / sizeof(config.driver_name[0]))) {
    if (is_altair() && n == 0)
      _tcscpy(config.driver_name, _T("Altair RU"));
    else if (is_altair() && n == 1)
      _tcscpy(config.driver_name, _T("Vega"));
    else
      config.driver_name[0] = '\0';
  }
}

static const TCHAR *
PortTypeToString(enum DeviceConfig::port_type type)
{
  switch (type) {
  case DeviceConfig::SERIAL:
    return _T("serial");

  case DeviceConfig::AUTO:
    return _T("auto");
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

  MakeDeviceSettingName(name, CONF("Port"), n, _T("Type"));
  return Profile::Set(name, value);
}

void
Profile::SetDeviceConfig(unsigned n, const DeviceConfig &config)
{
  TCHAR buffer[64];

  WritePortType(n, config.port_type);

  MakeDeviceSettingName(buffer, CONF("Port"), n, _T("Index"));
  Set(buffer, config.port_index);

  MakeDeviceSettingName(buffer, CONF("Speed"), n, _T("Index"));
  Set(buffer, config.speed_index);

  _tcscpy(buffer, CONF("DeviceA"));
  buffer[_tcslen(buffer) - 1] += n;
  Set(buffer, config.driver_name);
}
