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

#include "Internal.hpp"
#include "Device/Internal.hpp"
#include "NMEA/Derived.hpp"

#include <stdio.h>

#ifdef _UNICODE
#include <windows.h>
#endif

bool
VegaDevice::SendSetting(const char *name, int value)
{
  /* erase the old value from the settings map, because we expect to
     receive the new one now */
  settings_mutex.Lock();
  settings.erase(name);
  settings_mutex.Unlock();

  char buffer[64];
  sprintf(buffer, "PDVSC,S,%s,%d", name, value);
  return PortWriteNMEA(port, buffer);
}

bool
VegaDevice::RequestSetting(const char *name)
{
  char buffer[64];
  sprintf(buffer, "PDVSC,R,%s", name);
  return PortWriteNMEA(port, buffer);
}

std::pair<bool, int>
VegaDevice::GetSetting(const char *name) const
{
  ScopeLock protect(settings_mutex);
  auto i = settings.find(name);
  if (i == settings.end())
    return std::make_pair(false, 0);

  return std::make_pair(true, i->second);
}

void
VegaDevice::VarioWriteSettings(const DerivedInfo &calculated) const
{
    char mcbuf[100];

    sprintf(mcbuf, "PDVMC,%d,%d,%d,%d,%d",
            iround(calculated.common_stats.current_mc*10),
            iround(calculated.V_stf*10),
            calculated.circling,
            iround(calculated.terrain_altitude),
            uround(qnh.GetHectoPascal() * 10));

    PortWriteNMEA(port, mcbuf);
}

bool
VegaDevice::PutQNH(const AtmosphericPressure& pres, OperationEnvironment &env)
{
  qnh = pres;

  return true;
}
