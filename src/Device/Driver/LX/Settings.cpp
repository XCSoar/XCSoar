// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
LXDevice::SendNanoSetting(const char *name, unsigned value,
                          OperationEnvironment &env)
{
  char buffer[32];
  sprintf(buffer, "%u", value);
  return SendNanoSetting(name, buffer, env);
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

unsigned
LXDevice::GetNanoSettingInteger(const char *name) const noexcept
{
  const std::lock_guard<Mutex> lock{nano_settings};
  auto i = nano_settings.find(name);
  if (i == nano_settings.end())
    return {};

  return strtoul(i->c_str(), nullptr, 10);
}

bool
LXDevice::PutBallast([[maybe_unused]] double fraction, double overload,
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
