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

#include "Device/Windows/Enumerator.hpp"

gcc_pure
static bool
CompareRegistryValue(const RegistryKey &registry,
                     const TCHAR *name, const TCHAR *value)
{
  TCHAR real_value[64];
  return registry.get_value(name, real_value, 64) &&
    _tcsicmp(value, real_value) == 0;
}

gcc_pure
static bool
IsUnimodemPort(const RegistryKey &registry)
{
  return CompareRegistryValue(registry, _T("Tsp"), _T("Unimodem.dll"));
}

/**
 * Bluetooth driver found in hx4700.
 */
gcc_pure
static bool
IsWidcommSerialPort(const RegistryKey &registry)
{
  return CompareRegistryValue(registry, _T("Dll"), _T("btcedrivers.dll")) &&
    CompareRegistryValue(registry, _T("Prefix"), _T("COM"));
}

/**
 * Internal GPS found in Wayteq x950.
 */
gcc_pure
static bool
IsYFCommuxPort(const RegistryKey &registry)
{
  return CompareRegistryValue(registry, _T("Dll"), _T("commux.dll")) &&
    CompareRegistryValue(registry, _T("Prefix"), _T("COM"));
}

gcc_pure
static bool
IsGPSPort(const RegistryKey &registry)
{
  return CompareRegistryValue(registry, _T("Dll"), _T("GPS.Dll")) &&
    CompareRegistryValue(registry, _T("Prefix"), _T("COM"));
}

gcc_pure
static bool
IsSerialPort(const TCHAR *key)
{
  RegistryKey registry(HKEY_LOCAL_MACHINE, key, true);
  if (registry.error())
    return false;

  return IsUnimodemPort(registry) || IsWidcommSerialPort(registry) ||
    IsYFCommuxPort(registry) || IsGPSPort(registry);
}

gcc_pure
static bool
GetDeviceFriendlyName(const TCHAR *key, TCHAR *buffer, size_t max_size)
{
  RegistryKey registry(HKEY_LOCAL_MACHINE, key, true);
  return !registry.error() &&
    registry.get_value(_T("FriendlyName"), buffer, max_size);
}

PortEnumerator::PortEnumerator()
  :drivers_active(HKEY_LOCAL_MACHINE, _T("Drivers\\Active"), true),
   i(0)
{
}

bool
PortEnumerator::Next()
{
  assert(!drivers_active.error());

  TCHAR key_name[64];
  while (drivers_active.enum_key(i++, key_name, 64)) {
    RegistryKey device(drivers_active, key_name, true);
    if (device.error())
      continue;

    TCHAR device_key[64];
    if (device.get_value(_T("Key"), device_key, 64) &&
        IsSerialPort(device_key) &&
        device.get_value(_T("Name"), name.buffer(), name.MAX_SIZE)) {
      display_name = name;
      const size_t length = display_name.length();
      TCHAR *const tail = display_name.buffer() + length;
      const size_t remaining = display_name.MAX_SIZE - length - 3;

      if (GetDeviceFriendlyName(device_key, tail + 2, remaining)) {
        /* build a string in the form: "COM1: (Friendly Name)" */
        tail[0] = _T(' ');
        tail[1] = _T('(');
        _tcscat(tail, _T(")"));
      }

      return true;
    }
  }

  return false;
}
