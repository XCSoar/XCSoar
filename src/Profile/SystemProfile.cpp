// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SystemProfile.hpp"
#include "DeviceConfig.hpp"
#include "Map.hpp"
#include "Keys.hpp"
#include "SystemSettings.hpp"

void
Profile::Load(const ProfileMap &map, SystemSettings &settings)
{
  for (unsigned i = 0; i < settings.devices.size(); ++i)
    GetDeviceConfig(map, i, settings.devices[i]);
}
