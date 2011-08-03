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

#include "Profile/MapProfile.hpp"
#include "Profile/TerrainConfig.hpp"
#include "Profile/AirspaceConfig.hpp"
#include "Profile/Profile.hpp"
#include "SettingsMap.hpp"

void
Profile::Load(SETTINGS_MAP &settings)
{
  Get(szProfileCircleZoom, settings.CircleZoom);
  Get(szProfileMaxAutoZoomDistance, settings.MaxAutoZoomDistance);
  Get(szProfileDrawTopography, settings.EnableTopography);

  LoadTerrainRendererSettings(settings.terrain);

  GetEnum(szProfileAircraftSymbol, settings.aircraft_symbol);

  Get(szProfileTrailDrift, settings.EnableTrailDrift);
  Get(szProfileDetourCostMarker, settings.EnableDetourCostMarker);
  GetEnum(szProfileDisplayTrackBearing, settings.DisplayTrackBearing);
  Get(szProfileAutoZoom, settings.AutoZoom);
  Get(szProfileSnailWidthScale, settings.SnailScaling);

  GetEnum(szProfileSnailType, settings.SnailType);

  unsigned Temp;
  if (Get(szProfileWindArrowStyle, Temp))
    settings.WindArrowStyle = Temp;

  settings.waypoint.LoadFromProfile();

  GetEnum(szProfileSnailTrail, settings.trail_length);

  LoadAirspaceConfig();

  Get(szProfileGliderScreenPosition, settings.GliderScreenPosition);

  bool orientation_found = false;

  Temp = NORTHUP;
  if (Get(szProfileOrientationCircling, Temp))
    orientation_found = true;

  switch (Temp) {
  case TRACKUP:
    settings.OrientationCircling = TRACKUP;
    break;
  case NORTHUP:
    settings.OrientationCircling = NORTHUP;
    break;
  case TARGETUP:
    settings.OrientationCircling = TARGETUP;
    break;
  }

  Temp = NORTHUP;
  if (Get(szProfileOrientationCruise, Temp))
    orientation_found = true;

  switch (Temp) {
  case TRACKUP:
    settings.OrientationCruise = TRACKUP;
    break;
  case NORTHUP:
    settings.OrientationCruise = NORTHUP;
    break;
  case TARGETUP:
    settings.OrientationCruise = TARGETUP;
    break;
  }

  if (!orientation_found) {
    Temp = 1;
    Get(szProfileDisplayUpValue, Temp);
    switch (Temp) {
    case 0:
      settings.OrientationCruise = TRACKUP;
      settings.OrientationCircling = TRACKUP;
      break;
    case 1:
      settings.OrientationCruise = NORTHUP;
      settings.OrientationCircling = NORTHUP;
      break;
    case 2:
      settings.OrientationCruise = TRACKUP;
      settings.OrientationCircling = NORTHUP;
      break;
    case 3:
      settings.OrientationCruise = TRACKUP;
      settings.OrientationCircling = TARGETUP;
      break;
    case 4:
      settings.OrientationCruise = NORTHUP;
      settings.OrientationCircling = TRACKUP;
      break;
    }
  }

  GetEnum(szProfileMapShiftBias, settings.MapShiftBias);
  Get(szProfileEnableFLARMMap, settings.EnableFLARMMap);

  Get(szProfileEnableThermalProfile, settings.EnableThermalProfile);
}
