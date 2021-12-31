/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "LXNAVVario.hpp"

#include <cstdio>

bool
LXDevice::SendLXNAVVarioSetting(const char *name, const char *value,
                        OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  {
    const std::lock_guard<Mutex> lock(lxnav_vario_settings);
    lxnav_vario_settings.MarkOld(name);
  }

  char buffer[256];
  sprintf(buffer, "PLXV0,%s,W,%s", name, value);
  PortWriteNMEA(port, buffer, env);
  return true;
}

bool
LXDevice::RequestLXNAVVarioSetting(const char *name, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  {
    const std::lock_guard<Mutex> lock(lxnav_vario_settings);
    lxnav_vario_settings.MarkOld(name);
  }

  char buffer[256];
  sprintf(buffer, "PLXV0,%s,R", name);
  PortWriteNMEA(port, buffer, env);
  return true;
}

std::string
LXDevice::WaitLXNAVVarioSetting(const char *name, OperationEnvironment &env,
                        unsigned timeout_ms)
{
  std::unique_lock<Mutex> lock(lxnav_vario_settings);
  auto i = lxnav_vario_settings.Wait(lock, name, env,
                            std::chrono::milliseconds(timeout_ms));
  if (i == lxnav_vario_settings.end())
    return std::string();

  return *i;
}

std::string
LXDevice::GetLXNAVVarioSetting(const char *name) const noexcept
{
  std::lock_guard<Mutex> lock(lxnav_vario_settings);
  auto i = lxnav_vario_settings.find(name);
  if (i == lxnav_vario_settings.end())
    return std::string();

  return *i;
}

bool
LXDevice::SendNanoSetting(const char *name, const char *value,
                        OperationEnvironment &env)
{
  if (!EnableLoggerNMEA(env))
    return false;

  {
    const std::lock_guard<Mutex> lock(nano_settings);
    nano_settings.MarkOld(name);
  }

  char buffer[256];
  sprintf(buffer, "PLXVC,SET,W,%s,%s", name, value);
  PortWriteNMEA(port, buffer, env);
  return true;
}

bool
LXDevice::RequestNanoSetting(const char *name, OperationEnvironment &env)
{
  if (!EnableLoggerNMEA(env))
    return false;

  {
    const std::lock_guard<Mutex> lock(nano_settings);
    nano_settings.MarkOld(name);
  }

  char buffer[256];
  sprintf(buffer, "PLXVC,SET,R,%s", name);
  PortWriteNMEA(port, buffer, env);
  return true;
}

std::string
LXDevice::WaitNanoSetting(const char *name, OperationEnvironment &env,
                        unsigned timeout_ms)
{
  std::unique_lock<Mutex> lock(nano_settings);
  auto i = nano_settings.Wait(lock, name, env,
                              std::chrono::milliseconds(timeout_ms));
  if (i == nano_settings.end())
    return std::string();

  return *i;
}

std::string
LXDevice::GetNanoSetting(const char *name) const noexcept
{
  std::lock_guard<Mutex> lock(nano_settings);
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

  if (IsLXNAVVario())
    LXNAVVario::SetBallast(port, env, overload);
  else
    LX1600::SetBallast(port, env, overload);
  return true;
}

bool
LXDevice::PutBugs(double bugs, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  int transformed_bugs_value = 100 - (int)(bugs*100);

  if (IsLXNAVVario())
    LXNAVVario::SetBugs(port, env, transformed_bugs_value);
  else
    LX1600::SetBugs(port, env, transformed_bugs_value);
  return true;
}

bool
LXDevice::PutMacCready(double mac_cready, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  if (IsLXNAVVario())
    LXNAVVario::SetMacCready(port, env, mac_cready);
  else
    LX1600::SetMacCready(port, env, mac_cready);
  return true;
}

bool
LXDevice::PutQNH(const AtmosphericPressure &pres, OperationEnvironment &env)
{
  if (!EnableNMEA(env))
    return false;

  if (IsLXNAVVario())
    LXNAVVario::SetQNH(port, env, pres);
  else
    LX1600::SetQNH(port, env, pres);
  return true;
}

bool
LXDevice::PutVolume(unsigned volume, OperationEnvironment &env)
{
  if (!IsLX16xx() || !EnableNMEA(env))
    return false;

  LX1600::SetVolume(port, env, volume);
  return true;
}

bool
LXDevice::PutPilotEvent(OperationEnvironment &env)
{
  if (!IsLXNAVVario())
    return false;

  LXNAVVario::PutPilotEvent(env, port);
  return true;
}
