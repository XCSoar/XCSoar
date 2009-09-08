/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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

SettingsMapBlackboard::SettingsMapBlackboard()
{
  settings_map.CircleZoom = false;
  settings_map.ExtendedVisualGlide=false;
  settings_map.EnableTopology = false;
  settings_map.EnableTerrain = false;
  settings_map.DeclutterLabels = 0;
  settings_map.EnableTrailDrift = false;
  settings_map.EnableCDICruise = false;
  settings_map.EnableCDICircling = false;
  settings_map.AutoZoom = false;
  settings_map.SnailWidthScale = 16;
  settings_map.WindArrowStyle = 0;
  settings_map.DisplayTextType = DISPLAYNONE;
  settings_map.TrailActive = 1;
  settings_map.VisualGlide = 0;
  settings_map.bAirspaceBlackOutline = false;
  settings_map.GliderScreenPosition = 20; // 20% from bottom
  settings_map.DisplayOrientation = TRACKUP;
  settings_map.TerrainContrast = 150;
  settings_map.TerrainBrightness = 36;
  settings_map.TerrainRamp = 0;
  settings_map.OnAirSpace = 1;
  settings_map.EnableAuxiliaryInfo = 0;
  settings_map.UserForceDisplayMode = dmNone;
  settings_map.FullScreen = false;
  settings_map.EnablePan = false;
  settings_map.PanLongitude = 0;
  settings_map.PanLatitude = 0;
  settings_map.TargetPan = false;
  settings_map.TargetPanIndex = 0;
  settings_map.TargetZoomDistance = 500;
  settings_map.MapScale = 5;
  settings_map.EnableFLARMGauge = true;
  settings_map.EnableFLARMMap = 1;
  settings_map.ScreenBlanked = false;
  settings_map.EnableAutoBlank = false;
}
