// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Internal.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "Math/Util.hpp"
#include "Atmosphere/Pressure.hpp"

#include <stdio.h>

void
VegaDevice::SendSetting(const char *name, int value, OperationEnvironment &env)
{
  /* erase the old value from the settings map, because we expect to
     receive the new one now */
  {
      const std::lock_guard<Mutex> lock(settings);
      settings.erase(name);
  }

  char buffer[64];
  sprintf(buffer, "PDVSC,S,%s,%d", name, value);
  PortWriteNMEA(port, buffer, env);
}

void
VegaDevice::RequestSetting(const char *name, OperationEnvironment &env)
{
  char buffer[64];
  sprintf(buffer, "PDVSC,R,%s", name);
  PortWriteNMEA(port, buffer, env);
}

std::optional<int>
VegaDevice::GetSetting(const char *name) const noexcept
{
  std::lock_guard<Mutex> lock(settings);
  auto i = settings.find(name);
  if (i == settings.end())
    return std::nullopt;

  return *i;
}

bool
VegaDevice::PutMacCready(double _mc, OperationEnvironment &env)
{
  volatile_data.mc = uround(_mc * 10);
  volatile_data.SendTo(port, env);
  return true;
}

bool
VegaDevice::PutQNH(const AtmosphericPressure& pres, OperationEnvironment &env)
{
  volatile_data.qnh = uround(pres.GetHectoPascal() * 10);
  volatile_data.SendTo(port, env);
  return true;
}
