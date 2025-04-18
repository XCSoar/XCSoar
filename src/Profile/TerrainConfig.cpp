// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TerrainConfig.hpp"
#include "Keys.hpp"
#include "Map.hpp"
#include "Terrain/TerrainSettings.hpp"

void
Profile::LoadTerrainRendererSettings(const ProfileMap &map,
                                     TerrainRendererSettings &settings)
{
  map.Get(ProfileKeys::DrawTerrain, settings.enable);

  uint8_t Temp = (uint8_t)settings.slope_shading;
  if (!map.Get(ProfileKeys::SlopeShadingType, Temp)) {
    bool old_profile_setting = true;
    if (map.Get(ProfileKeys::SlopeShading, old_profile_setting))
      // 0: OFF, 3: Wind
      Temp = old_profile_setting ? 3 : 0;
  }
  settings.slope_shading = (SlopeShading)Temp;

  map.Get(ProfileKeys::TerrainContrast, settings.contrast);
  map.Get(ProfileKeys::TerrainBrightness, settings.brightness);

  unsigned short ramp;
  if (map.Get(ProfileKeys::TerrainRamp, ramp) &&
      ramp < TerrainRendererSettings::NUM_RAMPS)
    settings.ramp = ramp;

  uint8_t contours = (uint8_t)settings.contours;
  if (map.Get(ProfileKeys::TerrainContours, contours))
    settings.contours = (Contours)contours;
}
