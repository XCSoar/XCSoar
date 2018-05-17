/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "TerrainConfig.hpp"
#include "ProfileKeys.hpp"
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
