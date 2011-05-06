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

#include "SettingsMapBlackboard.hpp"
#include "Asset.hpp"

SettingsMapBlackboard::SettingsMapBlackboard()
{
  settings_map.CircleZoom = true;
  settings_map.MaxAutoZoomDistance = fixed(10000); /* 100 km */
  settings_map.EnableTopography = true;
  settings_map.NorthArrow = true;
  settings_map.terrain.SetDefaults();
  settings_map.EnableTrailDrift = true;
  settings_map.EnableDetourCostMarker = false;
  settings_map.DisplayTrackBearing = dtbAuto;
  settings_map.AutoZoom = false;
  settings_map.SnailScaling = true;
  settings_map.SnailType = stStandardVario;
  settings_map.WindArrowStyle = 0;
  settings_map.DisplayTextType = DISPLAYFIRSTFIVE;
  settings_map.WaypointArrivalHeightDisplay = WP_ARRIVAL_HEIGHT_GLIDE;
  settings_map.LandableRenderMode = RoundedBlack;
  settings_map.TrailActive = 1;
  settings_map.airspace.SetDefaults();
  settings_map.GliderScreenPosition = 20; // 20% from bottom
  settings_map.OrientationCircling = TRACKUP;
  settings_map.OrientationCruise = TRACKUP;
  settings_map.EnableAuxiliaryInfo = true;
  settings_map.AuxiliaryInfoBoxPanel = 0;
  settings_map.UserForceDisplayMode = dmNone;
  settings_map.EnablePan = false;
  settings_map.PanLocation.Longitude = Angle::native(fixed_zero);
  settings_map.PanLocation.Latitude = Angle::native(fixed_zero);
  settings_map.TargetPan = false;
  settings_map.TargetPanIndex = 0;
  settings_map.TargetZoomDistance = fixed(500);
  settings_map.EnableFLARMGauge = true;
  settings_map.AutoCloseFlarmDialog = false;
  settings_map.EnableFLARMMap = true;
  settings_map.ScreenBlanked = false;
  settings_map.EnableAutoBlank = false;

  settings_map.SetSystemTimeFromGPS = is_altair() && is_embedded();
}
