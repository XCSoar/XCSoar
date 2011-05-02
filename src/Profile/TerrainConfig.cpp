/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Profile/TerrainConfig.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Terrain/TerrainSettings.hpp"

void
Profile::LoadTerrainRendererSettings(TerrainRendererSettings &settings)
{
  Get(szProfileDrawTerrain, settings.enable);

  int Temp = settings.slope_shading;
  if (!Get(szProfileSlopeShadingType, Temp)) {
    bool old_profile_setting = true;
    if (Get(szProfileSlopeShading, old_profile_setting))
      // 0: OFF, 3: Wind
      Temp = old_profile_setting ? 3 : 0;
  }
  settings.slope_shading = (SlopeShadingType_t)Temp;

  Get(szProfileTerrainContrast, settings.contrast);
  Get(szProfileTerrainBrightness, settings.brightness);
  Get(szProfileTerrainRamp, settings.ramp);
}
