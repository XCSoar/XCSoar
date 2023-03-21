// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class ProfileMap;
struct SystemSettings;

namespace Profile {
  void Load(const ProfileMap &map, SystemSettings &settings);
};
