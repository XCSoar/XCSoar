// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class ProfileMap;
struct MapSettings;
struct TrailSettings;
struct MapItemListSettings;

namespace Profile {
  void Load(const ProfileMap &map, MapSettings &settings);
  void Load(const ProfileMap &map, TrailSettings &settings);
  void Load(const ProfileMap &map, MapItemListSettings &settings);
};
