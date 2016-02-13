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
#include "LX1600.hpp"
#include "V7.hpp"

#include <cstdio>

bool
LXDevice::SendV7Setting(const char *name, const char *value,
                        OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  v7_settings.Lock();
  v7_settings.MarkOld(name);
  v7_settings.Unlock();

  char buffer[256];
  sprintf(buffer, "PLXV0,%s,W,%s", name, value);
  return PortWriteNMEA(port, buffer, env);
}

bool
LXDevice::RequestV7Setting(const char *name, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  v7_settings.Lock();
  v7_settings.MarkOld(name);
  v7_settings.Unlock();

  char buffer[256];
  sprintf(buffer, "PLXV0,%s,R", name);
  return PortWriteNMEA(port, buffer, env);
}

std::string
LXDevice::WaitV7Setting(const char *name, OperationEnvironment &env,
                        unsigned timeout_ms)
{
  ScopeLock protect(v7_settings);
  auto i = v7_settings.Wait(name, env, timeout_ms);
  if (i == v7_settings.end())
    return std::string();

  return *i;
}

std::string
LXDevice::GetV7Setting(const char *name) const
{
  ScopeLock protect(v7_settings);
  auto i = v7_settings.find(name);
  if (i == v7_settings.end())
    return std::string();

  return *i;
}

bool
LXDevice::SendNanoSetting(const char *name, const char *value,
                        OperationEnvironment &env)
{
  if (!EnableNanoNMEA(env))
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
  if (!EnableNanoNMEA(env))
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
LXDevice::PutBallast(gcc_unused double fraction, double overload,
                     OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  if (IsV7())
    return V7::SetBallast(port, env, overload);
  else
    return LX1600::SetBallast(port, env, overload);
}

bool
LXDevice::PutBugs(double bugs, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  int transformed_bugs_value = 100 - (int)(bugs*100);

  if (IsV7())
    return V7::SetBugs(port, env, transformed_bugs_value);
  else
    return LX1600::SetBugs(port, env, transformed_bugs_value);
}

bool
LXDevice::PutMacCready(double mac_cready, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  if (IsV7())
    return V7::SetMacCready(port, env, mac_cready);
  else
    return LX1600::SetMacCready(port, env, mac_cready);
}

bool
LXDevice::PutQNH(const AtmosphericPressure &pres, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  if (IsV7())
    return V7::SetQNH(port, env, pres);
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
