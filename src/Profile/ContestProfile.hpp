// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct ContestSettings;
class ProfileMap;

namespace Profile {
  void Load(const ProfileMap &map, ContestSettings &settings);
};
