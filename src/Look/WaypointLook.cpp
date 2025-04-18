// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointLook.hpp"
#include "Renderer/WaypointRendererSettings.hpp"
#include "Resources.hpp"

void
WaypointLook::Initialise(const WaypointRendererSettings &settings,
                         const Font &_font, const Font &_bold_font)
{
  small_icon.LoadResource(IDB_SMALL_ALL);
  turn_point_icon.LoadResource(IDB_TURNPOINT_ALL);
  task_turn_point_icon.LoadResource(IDB_TASKTURNPOINT_ALL);
  mountain_top_icon.LoadResource(IDB_MOUNTAIN_TOP_ALL);
  mountain_pass_icon.LoadResource(IDB_MOUNTAIN_PASS_ALL);
  bridge_icon.LoadResource(IDB_BRIDGE_ALL);
  tunnel_icon.LoadResource(IDB_TUNNEL_ALL);
  tower_icon.LoadResource(IDB_TOWER_ALL);
  power_plant_icon.LoadResource(IDB_POWER_PLANT_ALL);
  obstacle_icon.LoadResource(IDB_OBSTACLE_ALL);
  thermal_hotspot_icon.LoadResource(IDB_THERMAL_HOTSPOT_ALL);
  marker_icon.LoadResource(IDB_MARK_ALL);
  vor_icon.LoadResource(IDB_VOR_ALL);
  ndb_icon.LoadResource(IDB_NDB_ALL);
  dam_icon.LoadResource(IDB_DAM_ALL);
  castle_icon.LoadResource(IDB_CASTLE_ALL);
  intersection_icon.LoadResource(IDB_INTERSECTION_ALL);
  reporting_point_icon.LoadResource(IDB_REPORTING_POINT_ALL);
  pgtakeoff_icon.LoadResource(IDB_PGTAKEOFF_ALL);
  pglanding_icon.LoadResource(IDB_PGLANDING_ALL);

  reachable_brush.Create(COLOR_GREEN);
  terrain_unreachable_brush.Create(LightColor(COLOR_RED));
  unreachable_brush.Create(COLOR_RED);
  white_brush.Create(COLOR_WHITE);
  light_gray_brush.Create(COLOR_LIGHT_GRAY);
  magenta_brush.Create(COLOR_MAGENTA);
  orange_brush.Create(COLOR_ORANGE);

  Reinitialise(settings);

  font = &_font;
  bold_font = &_bold_font;
}

void
WaypointLook::Reinitialise(const WaypointRendererSettings &settings)
{
  switch (settings.landable_style) {
  case WaypointRendererSettings::LandableStyle::PURPLE_CIRCLE:
    airport_reachable_icon.LoadResource(IDB_REACHABLE_ALL);
    airport_marginal_icon.LoadResource(IDB_MARGINAL_ALL);
    airport_unreachable_icon.LoadResource(IDB_LANDABLE_ALL);
    field_reachable_icon.LoadResource(IDB_REACHABLE_ALL);
    field_marginal_icon.LoadResource(IDB_MARGINAL_ALL);
    field_unreachable_icon.LoadResource(IDB_LANDABLE_ALL);
    break;

  case WaypointRendererSettings::LandableStyle::BW:
    airport_reachable_icon.LoadResource(IDB_AIRPORT_REACHABLE_ALL);
    airport_marginal_icon.LoadResource(IDB_AIRPORT_MARGINAL_ALL);
    airport_unreachable_icon.LoadResource(IDB_AIRPORT_UNREACHABLE_ALL);
    field_reachable_icon.LoadResource(IDB_OUTFIELD_REACHABLE_ALL);
    field_marginal_icon.LoadResource(IDB_OUTFIELD_MARGINAL_ALL);
    field_unreachable_icon.LoadResource(IDB_OUTFIELD_UNREACHABLE_ALL);
    break;

  case WaypointRendererSettings::LandableStyle::TRAFFIC_LIGHTS:
    airport_reachable_icon.LoadResource(IDB_AIRPORT_REACHABLE_ALL);
    airport_marginal_icon.LoadResource(IDB_AIRPORT_MARGINAL2_ALL);
    airport_unreachable_icon.LoadResource(IDB_AIRPORT_UNREACHABLE2_ALL);
    field_reachable_icon.LoadResource(IDB_OUTFIELD_REACHABLE_ALL);
    field_marginal_icon.LoadResource(IDB_OUTFIELD_MARGINAL2_ALL);
    field_unreachable_icon.LoadResource(IDB_OUTFIELD_UNREACHABLE2_ALL);
    break;
  }
}
