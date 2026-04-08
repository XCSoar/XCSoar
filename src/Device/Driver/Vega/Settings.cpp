// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Internal.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "Math/Util.hpp"
#include "Atmosphere/Pressure.hpp"

#include "lib/fmt/ToBuffer.hxx"

void
VegaDevice::SendSetting(const char *name, int value, OperationEnvironment &env)
{
  if (name == nullptr || name[0] == '\0')
    return;

  /* erase the old value from the settings map, because we expect to
     receive the new one now */
  {
    const std::lock_guard<Mutex> lock(settings);
    settings.erase(name);
  }

  const auto command = FmtBuffer<64>("PDVSC,S,{},{}", name, value);
  PortWriteNMEA(port, command.c_str(), env);
}

void
VegaDevice::RequestSetting(const char *name, OperationEnvironment &env)
{
  if (name == nullptr || name[0] == '\0')
    return;

  const auto command = FmtBuffer<64>("PDVSC,R,{}", name);
  PortWriteNMEA(port, command.c_str(), env);
}

std::optional<int>
VegaDevice::GetSetting(const char *name) const noexcept
{
  if (name == nullptr || name[0] == '\0')
    return std::nullopt;

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
