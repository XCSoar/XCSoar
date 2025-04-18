// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class ProfileMap;
struct TerrainRendererSettings;

namespace Profile
{
  void LoadTerrainRendererSettings(const ProfileMap &map,
                                   TerrainRendererSettings &settings);
};
