// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class ProfileMap;
struct ComputerSettings;
class RoughTimeDelta;

namespace Profile {
  bool LoadUTCOffset(const ProfileMap &map, RoughTimeDelta &value_r);
  void Load(const ProfileMap &map, ComputerSettings &settings);
};
