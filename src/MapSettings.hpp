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

#ifndef XCSOAR_MAP_SETTINGS_HPP
#define XCSOAR_MAP_SETTINGS_HPP

// changed only in config or by user interface
// not expected to be used by other threads

#include "Renderer/AirspaceRendererSettings.hpp"
#include "Renderer/WaypointRendererSettings.hpp"
#include "Engine/Task/Shapes/FAITriangleSettings.hpp"
#include "Terrain/TerrainSettings.hpp"

#include <type_traits>

#include <stdint.h>

enum class AircraftSymbol : uint8_t {
  SIMPLE,
  DETAILED,
  SIMPLE_LARGE,
  HANGGLIDER,
  PARAGLIDER,
};

enum class MapOrientation : uint8_t {
  TRACK_UP,
  NORTH_UP,
  TARGET_UP,
  HEADING_UP,
  WIND_UP,
};

enum class MapShiftBias : uint8_t {
  NONE,
  TRACK,
  TARGET
};

enum class DisplayGroundTrack: uint8_t {
  OFF,
  ON,
  AUTO,
};

enum class FinalGlideBarDisplayMode: uint8_t {
  OFF,
  ON,
  AUTO,
};

struct MapItemListSettings {

  /** Add an LocationMapItem to the MapItemList? */
  bool add_location;

  /** Add an ArrivalAltitudeMapItem to the MapItemList? */
  bool add_arrival_altitude;

  void SetDefaults();
};

static_assert(std::is_trivial<MapItemListSettings>::value, "type is not trivial");

struct TrailSettings {
  /** Snailtrail wind drifting in circling mode */
  bool wind_drift_enabled;
  bool scaling_enabled;

  /** 0: standard, 1: seeyou colors */
  enum class Type: uint8_t {
    VARIO_1,
    VARIO_2,
    ALTITUDE,
    VARIO_1_DOTS,
    VARIO_2_DOTS,
    VARIO_DOTS_AND_LINES,
  } type;

  enum class Length: uint8_t {
    OFF,
    LONG,
    SHORT,
    FULL,
  } length;

  void SetDefaults();
};

static_assert(std::is_trivial<TrailSettings>::value, "type is not trivial");

// user interface options

// where using these from Calculations or MapWindow thread, should
// protect

enum class WindArrowStyle: uint8_t {
  ARROW_HEAD,
  FULL_ARROW,
  NO_ARROW,
};

struct MapSettings {
  /** Map zooms in on circling */
  bool circle_zoom_enabled;
  /** Maximum distance limit for AutoZoom */
  double max_auto_zoom_distance;
  /** Map will show topography */
  bool topography_enabled;

  TerrainRendererSettings terrain;

  AircraftSymbol aircraft_symbol;

  /** Indicate extra distance reqd. if deviating from target heading */
  bool detour_cost_markers_enabled;
  /** Render track bearing on map */
  DisplayGroundTrack display_ground_track;

  /** Automatic zoom when closing in on waypoint */
  bool auto_zoom_enabled;
  WindArrowStyle wind_arrow_style;

  WaypointRendererSettings waypoint;
  AirspaceRendererSettings airspace;

  int glider_screen_position;
  /** Orientation of the map (North up, Track up, etc.) */
  MapOrientation cruise_orientation;
  MapOrientation circling_orientation;

  /** Map scale in cruise mode [px/m] */
  double cruise_scale;
  /** Map scale in circling mode [px/m] */
  double circling_scale;

  /** The bias for map shifting (Heading, Target, etc.) */
  MapShiftBias map_shift_bias;

  bool show_flarm_on_map;

  /**
   * This is an inverted copy of TrafficSettings::enable_gauge.  The
   * map should not render the FLARM alarm level if the gauge already
   * shows it, to declutter the map.  The copy is needed because
   * MapWindowBlackboard only knows MapSettings, but not
   * TrafficSettings, and DrawThread is not allowed to access
   * InterfaceBlackboard.
   */
  bool show_flarm_alarm_level;

  /** Display climb band on map */
  bool show_thermal_profile;

  /** Show FinalGlideBar mc0 arrow */
  bool final_glide_bar_mc0_enabled;

  /** FinalGlideBar display mode configuration */
  FinalGlideBarDisplayMode final_glide_bar_display_mode;

  /** Show Vario Bar arrow */
  bool vario_bar_enabled;

  /**
   * Overlay FAI triangle areas on the map while flying?
   */
  bool show_fai_triangle_areas;

  FAITriangleSettings fai_triangle_settings;

  TrailSettings trail;
  MapItemListSettings item_list;

  void SetDefaults();
};

static_assert(std::is_trivial<MapSettings>::value, "type is not trivial");

#endif
