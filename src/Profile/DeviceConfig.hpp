// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class ProfileMap;
struct DeviceConfig;

namespace Profile
{
  void GetDeviceConfig(const ProfileMap &map, unsigned n,
                       DeviceConfig &config);
  void SetDeviceConfig(ProfileMap &map, unsigned n,
                       const DeviceConfig &config);
};
