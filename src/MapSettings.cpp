// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapSettings.hpp"

void
MapItemListSettings::SetDefaults() noexcept
{
  add_location = true;
  add_arrival_altitude = true;
}

void
TrailSettings::SetDefaults() noexcept
{
  wind_drift_enabled = true;
  scaling_enabled = true;
  type = Type::VARIO_1;
  length = Length::LONG;
}

void
MapSettings::SetDefaults() noexcept
{
  circle_zoom_enabled = true;
  max_auto_zoom_distance = 100000; /* 100 km */
  topography_enabled = true;
  terrain.SetDefaults();
  aircraft_symbol = AircraftSymbol::SIMPLE;
  detour_cost_markers_enabled = false;
  display_ground_track = DisplayGroundTrack::AUTO;
  auto_zoom_enabled = false;
  wind_arrow_style = WindArrowStyle::ARROW_HEAD;
  waypoint.SetDefaults();
  airspace.SetDefaults();
  glider_screen_position = 20; // 20% from bottom
  map_shift_bias = MapShiftBias::NONE;
  circling_orientation = MapOrientation::NORTH_UP;
  cruise_orientation = MapOrientation::NORTH_UP;
  circling_scale = 0.5;
  cruise_scale = 1 / 60.;
  show_flarm_on_map = true;
  show_flarm_alarm_level = true;
  fade_traffic = true;
  show_thermal_profile = true;
  final_glide_bar_mc0_enabled = true;
  final_glide_bar_display_mode = FinalGlideBarDisplayMode::ON;
  vario_bar_enabled = false;
  show_fai_triangle_areas = false;
  skylines_traffic_map_mode = DisplaySkyLinesTrafficMapMode::SYMBOL;
  show_95_percent_rule_helpers = false;

  trail.SetDefaults();
  item_list.SetDefaults();
}
