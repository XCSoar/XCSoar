// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device.hpp"
#include "system/Sleep.h"

#include <stdio.h>

void
FlarmDevice::SendSetting(const char *name, const char *value,
                         OperationEnvironment &env)
{
  assert(name != nullptr);
  assert(value != nullptr);

  /* erase the old value from the settings map, because we expect to
     receive the new one now */
  {
    const std::lock_guard<Mutex> lock(settings);
    settings.erase(name);
  }

  char buffer[64];
  sprintf(buffer, "PFLAC,S,%s,%s", name, value);
  Send(buffer, env);
}

bool
FlarmDevice::RequestAllSettings(const char* const* settings, 
                                OperationEnvironment &env)
{
  try {
    for (auto i = settings; *i != NULL; ++i)
      FlarmDevice::RequestSetting(*i, env);

    for (auto i = settings; *i != NULL; ++i)
      FlarmDevice::WaitForSetting(*i, 500);
  } catch (OperationCancelled) {
    return false;
  } catch (...) {
    env.SetError(std::current_exception());
    return false;
  }

  return true;
}

void
FlarmDevice::RequestSetting(const char *name, OperationEnvironment &env)
{
  char buffer[64];
  sprintf(buffer, "PFLAC,R,%s", name);
  Send(buffer, env);
}

bool
FlarmDevice::WaitForSetting(const char *name, unsigned timeout_ms)
{
  for (unsigned i = 0; i < timeout_ms / 100; ++i) {
    if (FlarmDevice::SettingExists(name))
      return true;
    Sleep(100);
  }

  return false;
}

[[gnu::pure]]
bool
FlarmDevice::SettingExists(const char *name) noexcept
{
  return (bool)FlarmDevice::GetSetting(name);
}

std::optional<std::string>
FlarmDevice::GetSetting(const char *name) const noexcept
{
  std::lock_guard<Mutex> lock(settings);
  auto i = settings.find(name);
  if (i == settings.end())
    return std::nullopt;

  return *i;
}

unsigned
FlarmDevice::GetUnsignedValue(const char *name, unsigned default_value)
{
  if (const auto x = FlarmDevice::GetSetting(name)) {
    char *endptr;
    unsigned long y = strtoul(x->c_str(), &endptr, 10);
    if (endptr > x->c_str() && *endptr == 0)
      return (unsigned)y;
  }

  return default_value;
}
