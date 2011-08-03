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
#ifndef XCSOAR_SETTINGS_USER_HPP
#define XCSOAR_SETTINGS_USER_HPP

// changed only in config or by user interface
// not expected to be used by other threads

#include "Navigation/GeoPoint.hpp"
#include "Airspace/AirspaceClass.hpp"
#include "Airspace/AirspaceRendererSettings.hpp"
#include "Waypoint/WaypointRendererSettings.hpp"
#include "Terrain/TerrainSettings.hpp"
#include "DisplayMode.hpp"

enum AircraftSymbol_t {
  acSimple = 0,
  acDetailed,
  acSimpleLarge,
};

enum DisplayOrientation_t {
  TRACKUP = 0,
  NORTHUP,
  TARGETUP
};

enum MapShiftBias_t {
  MAP_SHIFT_BIAS_NONE = 0,
  MAP_SHIFT_BIAS_TRACK,
  MAP_SHIFT_BIAS_TARGET
};

enum TrailLength {
  TRAIL_OFF,
  TRAIL_LONG,
  TRAIL_SHORT,
  TRAIL_FULL,
};

enum SnailType_t {
  stStandardVario,
  stSeeYouVario,
  stAltitude,
};

enum DisplayTrackBearing_t {
  dtbOff,
  dtbOn,
  dtbAuto
};

/** Location of Flarm radar */
enum FlarmLocation {
  flAuto,
  flTopLeft,
  flTopRight,
  flBottomLeft,
  flBottomRight,
  flCentreTop,
  flCentreBottom,
};

// user interface options

// where using these from Calculations or MapWindow thread, should
// protect

struct SETTINGS_MAP {
  /** Map zooms in on circling */
  bool CircleZoom;
  /** Maximum distance limit for AutoZoom */
  fixed MaxAutoZoomDistance;
  /** Map will show topography */
  bool EnableTopography;

  TerrainRendererSettings terrain;

  AircraftSymbol_t aircraft_symbol;

  /** Snailtrail wind drifting in circling mode */
  bool EnableTrailDrift;
  /** Indicate extra distance reqd. if deviating from target heading */
  bool EnableDetourCostMarker;
  /** Render track bearing on map */
  DisplayTrackBearing_t DisplayTrackBearing;

  /** Automatic zoom when closing in on waypoint */
  bool AutoZoom;
  bool SnailScaling;
  /** 0: standard, 1: seeyou colors */
  SnailType_t SnailType;
  int WindArrowStyle;

  WaypointRendererSettings waypoint;

  TrailLength trail_length;

  AirspaceRendererSettings airspace;

  int GliderScreenPosition;
  /** Orientation of the map (North up, Track up, etc.) */
  DisplayOrientation_t OrientationCruise;
  DisplayOrientation_t OrientationCircling;

  /** The bias for map shifting (Heading, Target, etc.) */
  MapShiftBias_t MapShiftBias;

  bool EnableAuxiliaryInfo;
  unsigned AuxiliaryInfoBoxPanel;

  DisplayMode UserForceDisplayMode;

  unsigned TargetPanIndex;
  fixed TargetZoomDistance;
  /** Show FLARM radar if traffic present */
  bool EnableFLARMGauge;
  /** Automatically close the FLARM dialog when no traffic present */
  bool AutoCloseFlarmDialog;
  /** Show ThermalAssistant if circling */
  bool EnableTAGauge;
  bool EnableFLARMMap;
  bool ScreenBlanked;
  bool EnableAutoBlank;
  /** Update system time from GPS time */
  bool SetSystemTimeFromGPS;

  /** Display climb band on map */
  bool EnableThermalProfile;
};

#endif
