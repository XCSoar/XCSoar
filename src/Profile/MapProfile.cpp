/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "MapSettings.hpp"

void
Profile::Load(MapSettings &settings)
{
  Get(szProfileCircleZoom, settings.circle_zoom_enabled);
  Get(szProfileMaxAutoZoomDistance, settings.max_auto_zoom_distance);
  Get(szProfileDrawTopography, settings.topography_enabled);

  LoadTerrainRendererSettings(settings.terrain);

  GetEnum(szProfileAircraftSymbol, settings.aircraft_symbol);

  Get(szProfileDetourCostMarker, settings.detour_cost_markers_enabled);
  GetEnum(szProfileDisplayTrackBearing, settings.display_track_bearing);
  Get(szProfileAutoZoom, settings.auto_zoom_enabled);

  GetEnum(szProfileWindArrowStyle, settings.wind_arrow_style);

  settings.waypoint.LoadFromProfile();

  Load(settings.airspace);

  Get(szProfileGliderScreenPosition, settings.glider_screen_position);

  bool orientation_found = false;

  unsigned Temp = NORTHUP;
  if (Get(szProfileOrientationCircling, Temp))
    orientation_found = true;

  switch (Temp) {
  case TRACKUP:
    settings.circling_orientation = TRACKUP;
    break;
  case NORTHUP:
    settings.circling_orientation = NORTHUP;
    break;
  case TARGETUP:
    settings.circling_orientation = TARGETUP;
    break;
  }

  Temp = NORTHUP;
  if (Get(szProfileOrientationCruise, Temp))
    orientation_found = true;

  switch (Temp) {
  case TRACKUP:
    settings.cruise_orientation = TRACKUP;
    break;
  case NORTHUP:
    settings.cruise_orientation = NORTHUP;
    break;
  case TARGETUP:
    settings.cruise_orientation = TARGETUP;
    break;
  }

  if (!orientation_found) {
    Temp = 1;
    Get(szProfileDisplayUpValue, Temp);
    switch (Temp) {
    case 0:
      settings.cruise_orientation = TRACKUP;
      settings.circling_orientation = TRACKUP;
      break;
    case 1:
      settings.cruise_orientation = NORTHUP;
      settings.circling_orientation = NORTHUP;
      break;
    case 2:
      settings.cruise_orientation = TRACKUP;
      settings.circling_orientation = NORTHUP;
      break;
    case 3:
      settings.cruise_orientation = TRACKUP;
      settings.circling_orientation = TARGETUP;
      break;
    case 4:
      settings.cruise_orientation = NORTHUP;
      settings.circling_orientation = TRACKUP;
      break;
    }
  }

  fixed tmp;
  if (Profile::Get(szProfileClimbMapScale, tmp))
    settings.circling_scale =
      std::max(fixed(0.0003), std::min(tmp / 10000, fixed(10)));

  if (Profile::Get(szProfileCruiseMapScale, tmp))
    settings.cruise_scale =
      std::max(fixed(0.0003), std::min(tmp / 10000, fixed(10)));

  GetEnum(szProfileMapShiftBias, settings.map_shift_bias);
  Get(szProfileEnableFLARMMap, settings.show_flarm_on_map);

  Get(szProfileEnableThermalProfile, settings.show_thermal_profile);

  Load(settings.trail);
  Load(settings.item_list);
}

void
Profile::Load(TrailSettings &settings)
{
  Get(szProfileTrailDrift, settings.wind_drift_enabled);
  Get(szProfileSnailWidthScale, settings.scaling_enabled);
  GetEnum(szProfileSnailType, settings.type);
  GetEnum(szProfileSnailTrail, settings.length);
}

void
Profile::Load(MapItemListSettings &settings)
{
  Get(EnableLocationMapItemProfileKey, settings.add_location);
  Get(EnableArrivalAltitudeMapItemProfileKey, settings.add_arrival_altitude);
}
