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
  settings_map.CircleZoom = false;
  settings_map.MaxAutoZoomDistance = fixed(10000); /* 100 km */
  settings_map.EnableTopography = true;
  settings_map.NorthArrow = true;
  settings_map.terrain.SetDefaults();
  settings_map.EnableTrailDrift = false;
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
  settings_map.bAirspaceBlackOutline = false;
#ifndef ENABLE_OPENGL
  settings_map.airspace_transparency = false;
  settings_map.AirspaceFillMode = SETTINGS_MAP::AS_FILL_DEFAULT;
#endif
  settings_map.GliderScreenPosition = 20; // 20% from bottom
  settings_map.OrientationCircling = TRACKUP;
  settings_map.OrientationCruise = TRACKUP;
  settings_map.EnableAirspace = true;
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
  settings_map.iAirspaceBrush[0]=2;
  settings_map.iAirspaceBrush[1]=0;
  settings_map.iAirspaceBrush[2]=0;
  settings_map.iAirspaceBrush[3]=0;
  settings_map.iAirspaceBrush[4]=3;
  settings_map.iAirspaceBrush[5]=3;
  settings_map.iAirspaceBrush[6]=3;
  settings_map.iAirspaceBrush[7]=3;
  settings_map.iAirspaceBrush[8]=0;
  settings_map.iAirspaceBrush[9]=3;
  settings_map.iAirspaceBrush[10]=2;
  settings_map.iAirspaceBrush[11]=3;
  settings_map.iAirspaceBrush[12]=3;
  settings_map.iAirspaceBrush[13]=3;

  settings_map.iAirspaceColour[ 0]= 5;
  settings_map.iAirspaceColour[ 1]= 0;
  settings_map.iAirspaceColour[ 2]= 0;
  settings_map.iAirspaceColour[ 3]= 10;
  settings_map.iAirspaceColour[ 4]= 0;
  settings_map.iAirspaceColour[ 5]= 0;
  settings_map.iAirspaceColour[ 6]= 10;
  settings_map.iAirspaceColour[ 7]= 2;
  settings_map.iAirspaceColour[ 8]= 0;
  settings_map.iAirspaceColour[ 9]= 10;
  settings_map.iAirspaceColour[10]= 9;
  settings_map.iAirspaceColour[11]= 3;
  settings_map.iAirspaceColour[12]= 7;
  settings_map.iAirspaceColour[13]= 7;
}
