/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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
#include "Device/Port/Port.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "LX1600.hpp"
#include "Vario.hpp"

#include <cstdio>

bool
LXDevice::SendVarioSetting(const char *name, const char *value,
                        OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  vario_settings.Lock();
  vario_settings.MarkOld(name);
  vario_settings.Unlock();

  char buffer[256];
  sprintf(buffer, "PLXV0,%s,W,%s", name, value);
  return PortWriteNMEA(port, buffer, env);
}

bool
LXDevice::RequestVarioSetting(const char *name, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  vario_settings.Lock();
  vario_settings.MarkOld(name);
  vario_settings.Unlock();

  char buffer[256];
  sprintf(buffer, "PLXV0,%s,R", name);
  return PortWriteNMEA(port, buffer, env);
}

std::string
LXDevice::WaitVarioSetting(const char *name, OperationEnvironment &env,
                        unsigned timeout_ms)
{
  ScopeLock protect(vario_settings);
  auto i = vario_settings.Wait(name, env, timeout_ms);
  if (i == vario_settings.end())
    return std::string();

  return *i;
}

std::string
LXDevice::GetVarioSetting(const char *name) const
{
  ScopeLock protect(vario_settings);
  auto i = vario_settings.find(name);
  if (i == vario_settings.end())
    return std::string();

  return *i;
}

bool
LXDevice::SendNanoSetting(const char *name, const char *value,
                        OperationEnvironment &env)
{
  if (!EnableProperNMEA(env))
    return false;

  nano_settings.Lock();
  nano_settings.MarkOld(name);
  nano_settings.Unlock();

  char buffer[256];
  sprintf(buffer, "PLXVC,SET,W,%s,%s", name, value);
  return PortWriteNMEA(port, buffer, env);
}

bool
LXDevice::RequestNanoSetting(const char *name, OperationEnvironment &env)
{
  if (!EnableProperNMEA(env))
    return false;

  nano_settings.Lock();
  nano_settings.MarkOld(name);
  nano_settings.Unlock();

  char buffer[256];
  sprintf(buffer, "PLXVC,SET,R,%s", name);
  return PortWriteNMEA(port, buffer, env);
}

std::string
LXDevice::WaitNanoSetting(const char *name, OperationEnvironment &env,
                        unsigned timeout_ms)
{
  ScopeLock protect(nano_settings);
  auto i = nano_settings.Wait(name, env, timeout_ms);
  if (i == nano_settings.end())
    return std::string();

  return *i;
}

std::string
LXDevice::GetNanoSetting(const char *name) const
{
  ScopeLock protect(nano_settings);
  auto i = nano_settings.find(name);
  if (i == nano_settings.end())
    return std::string();

  return *i;
}

bool
LXDevice::PutBallast(gcc_unused fixed fraction, fixed overload,
                     OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  if (IsV7() || IsSVario())
    return Vario::SetBallast(port, env, overload);
  else
    return LX1600::SetBallast(port, env, overload);
}

bool
LXDevice::PutBugs(fixed bugs, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  int transformed_bugs_value = 100 - (int)(bugs*100);

  if (IsV7() || IsSVario())
    return Vario::SetBugs(port, env, transformed_bugs_value);
  else
    return LX1600::SetBugs(port, env, transformed_bugs_value);
}

bool
LXDevice::PutMacCready(fixed mac_cready, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  if (IsV7() || IsSVario())
    return Vario::SetMacCready(port, env, mac_cready);
  else
    return LX1600::SetMacCready(port, env, mac_cready);
}

bool
LXDevice::PutQNH(const AtmosphericPressure &pres, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  if (IsV7() || IsSVario())
    return Vario::SetQNH(port, env, pres);
  else
    return LX1600::SetQNH(port, env, pres);
}

bool
LXDevice::PutVolume(unsigned volume, OperationEnvironment &env)
{
  if (!IsLX16xx() || !EnableNMEA(env))
    return false;

  return LX1600::SetVolume(port, env, volume);
}
