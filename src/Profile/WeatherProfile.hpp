// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct WeatherSettings;
class ProfileMap;

namespace Profile {
  void Load(const ProfileMap &map, WeatherSettings &settings);
};
