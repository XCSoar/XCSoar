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
#include "Renderer/AirspaceRendererSettings.hpp"
#include "Renderer/WaypointRendererSettings.hpp"
#include "Terrain/TerrainSettings.hpp"

enum AircraftSymbol {
  acSimple = 0,
  acDetailed,
  acSimpleLarge,
};

enum DisplayOrientation {
  TRACKUP = 0,
  NORTHUP,
  TARGETUP
};

enum MapShiftBias {
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

enum SnailType {
  stStandardVario,
  stSeeYouVario,
  stAltitude,
};

enum DisplayTrackBearing {
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
  bool circle_zoom_enabled;
  /** Maximum distance limit for AutoZoom */
  fixed max_auto_zoom_distance;
  /** Map will show topography */
  bool topography_enabled;

  TerrainRendererSettings terrain;

  AircraftSymbol aircraft_symbol;

  /** Snailtrail wind drifting in circling mode */
  bool trail_drift_enabled;
  /** Indicate extra distance reqd. if deviating from target heading */
  bool detour_cost_markers_enabled;
  /** Render track bearing on map */
  DisplayTrackBearing display_track_bearing;

  /** Automatic zoom when closing in on waypoint */
  bool auto_zoom_enabled;
  bool snail_scaling_enabled;
  /** 0: standard, 1: seeyou colors */
  SnailType snail_type;
  int wind_arrow_style;

  WaypointRendererSettings waypoint;

  TrailLength trail_length;

  AirspaceRendererSettings airspace;

  int glider_screen_position;
  /** Orientation of the map (North up, Track up, etc.) */
  DisplayOrientation cruise_orientation;
  DisplayOrientation circling_orientation;

  /** The bias for map shifting (Heading, Target, etc.) */
  MapShiftBias map_shift_bias;

  bool show_flarm_on_map;

  /** Display climb band on map */
  bool show_thermal_profile;

  void SetDefaults();
};

#endif
