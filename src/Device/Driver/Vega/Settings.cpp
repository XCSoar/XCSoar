/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Device/Util/NMEAWriter.hpp"
#include "Math/Util.hpp"
#include "Atmosphere/Pressure.hpp"

#include <stdio.h>

#ifdef _UNICODE
#include <windows.h>
#endif

bool
VegaDevice::SendSetting(const char *name, int value, OperationEnvironment &env)
{
  /* erase the old value from the settings map, because we expect to
     receive the new one now */
  settings.Lock();
  settings.erase(name);
  settings.Unlock();

  char buffer[64];
  sprintf(buffer, "PDVSC,S,%s,%d", name, value);
  return PortWriteNMEA(port, buffer, env);
}

bool
VegaDevice::RequestSetting(const char *name, OperationEnvironment &env)
{
  char buffer[64];
  sprintf(buffer, "PDVSC,R,%s", name);
  return PortWriteNMEA(port, buffer, env);
}

std::pair<bool, int>
VegaDevice::GetSetting(const char *name) const
{
  ScopeLock protect(settings);
  auto i = settings.find(name);
  if (i == settings.end())
    return std::make_pair(false, 0);

  return std::make_pair(true, *i);
}

bool
VegaDevice::PutMacCready(double _mc, OperationEnvironment &env)
{
  volatile_data.mc = uround(_mc * 10);
  return volatile_data.SendTo(port, env);
}

bool
VegaDevice::PutQNH(const AtmosphericPressure& pres, OperationEnvironment &env)
{
  volatile_data.qnh = uround(pres.GetHectoPascal() * 10);
  return volatile_data.SendTo(port, env);
}
