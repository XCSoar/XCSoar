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

#include "WaypointLook.hpp"
#include "Renderer/WaypointRendererSettings.hpp"
#include "Resources.hpp"

void
WaypointLook::Initialise(const WaypointRendererSettings &settings,
                         const Font &_font, const Font &_bold_font)
{

  small_icon.LoadResource(IDB_SMALL, IDB_SMALL_HD);
  turn_point_icon.LoadResource(IDB_TURNPOINT, IDB_TURNPOINT_HD);
  task_turn_point_icon.LoadResource(IDB_TASKTURNPOINT, IDB_TASKTURNPOINT_HD);
  mountain_top_icon.LoadResource(IDB_MOUNTAIN_TOP, IDB_MOUNTAIN_TOP_HD);
  mountain_pass_icon.LoadResource(IDB_MOUNTAIN_PASS, IDB_MOUNTAIN_PASS_HD);
  bridge_icon.LoadResource(IDB_BRIDGE, IDB_BRIDGE_HD);
  tunnel_icon.LoadResource(IDB_TUNNEL, IDB_TUNNEL_HD);
  tower_icon.LoadResource(IDB_TOWER, IDB_TOWER_HD);
  power_plant_icon.LoadResource(IDB_POWER_PLANT, IDB_POWER_PLANT_HD);
  obstacle_icon.LoadResource(IDB_OBSTACLE, IDB_OBSTACLE_HD);
  thermal_hotspot_icon.LoadResource(IDB_THERMAL_HOTSPOT, IDB_THERMAL_HOTSPOT_HD);
  marker_icon.LoadResource(IDB_MARK, IDB_MARK_HD);

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
    airport_reachable_icon.LoadResource(IDB_REACHABLE, IDB_REACHABLE_HD);
    airport_marginal_icon.LoadResource(IDB_MARGINAL, IDB_MARGINAL_HD);
    airport_unreachable_icon.LoadResource(IDB_LANDABLE, IDB_LANDABLE_HD);
    field_reachable_icon.LoadResource(IDB_REACHABLE, IDB_REACHABLE_HD);
    field_marginal_icon.LoadResource(IDB_MARGINAL, IDB_MARGINAL_HD);
    field_unreachable_icon.LoadResource(IDB_LANDABLE, IDB_LANDABLE_HD);
    break;

  case WaypointRendererSettings::LandableStyle::BW:
    airport_reachable_icon.LoadResource(IDB_AIRPORT_REACHABLE,
                                        IDB_AIRPORT_REACHABLE_HD);
    airport_marginal_icon.LoadResource(IDB_AIRPORT_MARGINAL,
                                       IDB_AIRPORT_MARGINAL_HD);
    airport_unreachable_icon.LoadResource(IDB_AIRPORT_UNREACHABLE,
                                          IDB_AIRPORT_UNREACHABLE_HD);
    field_reachable_icon.LoadResource(IDB_OUTFIELD_REACHABLE,
                                      IDB_OUTFIELD_REACHABLE_HD);
    field_marginal_icon.LoadResource(IDB_OUTFIELD_MARGINAL,
                                     IDB_OUTFIELD_MARGINAL_HD);
    field_unreachable_icon.LoadResource(IDB_OUTFIELD_UNREACHABLE,
                                        IDB_OUTFIELD_UNREACHABLE_HD);
    break;

  case WaypointRendererSettings::LandableStyle::TRAFFIC_LIGHTS:
    airport_reachable_icon.LoadResource(IDB_AIRPORT_REACHABLE,
                                        IDB_AIRPORT_REACHABLE_HD);
    airport_marginal_icon.LoadResource(IDB_AIRPORT_MARGINAL2,
                                       IDB_AIRPORT_MARGINAL2_HD);
    airport_unreachable_icon.LoadResource(IDB_AIRPORT_UNREACHABLE2,
                                          IDB_AIRPORT_UNREACHABLE2_HD);
    field_reachable_icon.LoadResource(IDB_OUTFIELD_REACHABLE,
                                      IDB_OUTFIELD_REACHABLE_HD);
    field_marginal_icon.LoadResource(IDB_OUTFIELD_MARGINAL2,
                                     IDB_OUTFIELD_MARGINAL2_HD);
    field_unreachable_icon.LoadResource(IDB_OUTFIELD_UNREACHABLE2,
                                        IDB_OUTFIELD_UNREACHABLE2_HD);
    break;
  }
}
