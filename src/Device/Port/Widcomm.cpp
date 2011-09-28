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

#include "Widcomm.hpp"
#include "Config/Registry.hpp"

static bool
FindDevice(const TCHAR *name, TCHAR *result, size_t result_size)
{
  RegistryKey drivers_active(HKEY_LOCAL_MACHINE, _T("Drivers\\Active"), true);
  if (drivers_active.error())
    return false;

  TCHAR key_name[64], value[64];
  for (unsigned i = 0; drivers_active.enum_key(i, key_name, 64); ++i) {
    RegistryKey device(drivers_active, key_name, true);
    if (!device.error() && device.get_value(_T("Name"), value, 64) &&
        _tcscmp(name, value) == 0)
      return device.get_value(_T("Key"), result, result_size);
  }

  return false;
}

bool
IsWidcommDevice(const TCHAR *name)
{
  TCHAR key[64];
  if (!FindDevice(name, key, 64))
    return false;

  RegistryKey registry(HKEY_LOCAL_MACHINE, key, true);
  if (registry.error())
    return false;

  TCHAR dll[64];
  return registry.get_value(_T("Dll"), dll, 64) &&
    _tcscmp(dll, _T("btcedrivers.dll"));
}

