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
