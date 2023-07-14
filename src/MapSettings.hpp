// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

// changed only in config or by user interface
// not expected to be used by other threads

#include "Renderer/AirspaceRendererSettings.hpp"
#include "Renderer/WaypointRendererSettings.hpp"
#include "Engine/Task/Shapes/FAITriangleSettings.hpp"
#include "Terrain/TerrainSettings.hpp"

#include <type_traits>

#include <cstdint>

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

enum class DisplaySkyLinesTrafficMapMode: uint8_t {
  OFF,
  SYMBOL,
  SYMBOL_NAME,
};

struct MapItemListSettings {

  /** Add an LocationMapItem to the MapItemList? */
  bool add_location;

  /** Add an ArrivalAltitudeMapItem to the MapItemList? */
  bool add_arrival_altitude;

  void SetDefaults() noexcept;
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
    VARIO_EINK,
  } type;

  enum class Length: uint8_t {
    OFF,
    LONG,
    SHORT,
    FULL,
  } length;

  void SetDefaults() noexcept;
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

  /**
   * Keep showing traffic for a while after it has disappeared?
   */
  bool fade_traffic;

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

  /**
   * Display skylines name on map
   */
  DisplaySkyLinesTrafficMapMode skylines_traffic_map_mode;

  FAITriangleSettings fai_triangle_settings;

  TrailSettings trail;
  MapItemListSettings item_list;

  /** Show 95% distance rule helpers on map and infoboxes */
  bool show_95_percent_rule_helpers;

  void SetDefaults() noexcept;
};

static_assert(std::is_trivial<MapSettings>::value, "type is not trivial");
