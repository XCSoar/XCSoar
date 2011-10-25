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

#include "WaypointLook.hpp"
#include "Renderer/WaypointRendererSettings.hpp"
#include "resource.h"

void
WaypointLook::Initialise(const WaypointRendererSettings &settings)
{
  small_icon.Load(IDB_SMALL, IDB_SMALL_HD);
  turn_point_icon.Load(IDB_TURNPOINT, IDB_TURNPOINT_HD);
  task_turn_point_icon.Load(IDB_TASKTURNPOINT, IDB_TASKTURNPOINT_HD);
  mountain_top_icon.Load(IDB_MOUNTAIN_TOP, IDB_MOUNTAIN_TOP_HD);
  mountain_pass_icon.Load(IDB_MOUNTAIN_PASS, IDB_MOUNTAIN_PASS_HD);
  bridge_icon.Load(IDB_BRIDGE, IDB_BRIDGE_HD);
  tunnel_icon.Load(IDB_TUNNEL, IDB_TUNNEL_HD);
  tower_icon.Load(IDB_TOWER, IDB_TOWER_HD);
  power_plant_icon.Load(IDB_POWER_PLANT, IDB_POWER_PLANT_HD);
  obstacle_icon.Load(IDB_OBSTACLE, IDB_OBSTACLE_HD);

  reachable_brush.Set(COLOR_GREEN);
  terrain_unreachable_brush.Set(LightColor(COLOR_RED));
  unreachable_brush.Set(COLOR_RED);
  white_brush.Set(COLOR_WHITE);
  light_gray_brush.Set(COLOR_LIGHT_GRAY);
  magenta_brush.Set(COLOR_MAGENTA);
  orange_brush.Set(COLOR_ORANGE);

  if (settings.landable_style == wpLandableWinPilot) {
    airport_reachable_icon.Load(IDB_REACHABLE, IDB_REACHABLE_HD);
    airport_marginal_icon.Load(IDB_MARGINAL, IDB_MARGINAL_HD);
    airport_unreachable_icon.Load(IDB_LANDABLE, IDB_LANDABLE_HD);
    field_reachable_icon.Load(IDB_REACHABLE, IDB_REACHABLE_HD);
    field_marginal_icon.Load(IDB_MARGINAL, IDB_MARGINAL_HD);
    field_unreachable_icon.Load(IDB_LANDABLE, IDB_LANDABLE_HD);
  } else if (settings.landable_style == wpLandableAltA) {
    airport_reachable_icon.Load(IDB_AIRPORT_REACHABLE,
                                    IDB_AIRPORT_REACHABLE_HD);
    airport_marginal_icon.Load(IDB_AIRPORT_MARGINAL,
                                   IDB_AIRPORT_MARGINAL_HD);
    airport_unreachable_icon.Load(IDB_AIRPORT_UNREACHABLE,
                                      IDB_AIRPORT_UNREACHABLE_HD);
    field_reachable_icon.Load(IDB_OUTFIELD_REACHABLE,
                                  IDB_OUTFIELD_REACHABLE_HD);
    field_marginal_icon.Load(IDB_OUTFIELD_MARGINAL,
                                 IDB_OUTFIELD_MARGINAL_HD);
    field_unreachable_icon.Load(IDB_OUTFIELD_UNREACHABLE,
                                    IDB_OUTFIELD_UNREACHABLE_HD);
  } else if (settings.landable_style == wpLandableAltB) {
    airport_reachable_icon.Load(IDB_AIRPORT_REACHABLE,
                                    IDB_AIRPORT_REACHABLE_HD);
    airport_marginal_icon.Load(IDB_AIRPORT_MARGINAL2,
                                   IDB_AIRPORT_MARGINAL2_HD);
    airport_unreachable_icon.Load(IDB_AIRPORT_UNREACHABLE2,
                                      IDB_AIRPORT_UNREACHABLE2_HD);
    field_reachable_icon.Load(IDB_OUTFIELD_REACHABLE,
                                  IDB_OUTFIELD_REACHABLE_HD);
    field_marginal_icon.Load(IDB_OUTFIELD_MARGINAL2,
                                 IDB_OUTFIELD_MARGINAL2_HD);
    field_unreachable_icon.Load(IDB_OUTFIELD_UNREACHABLE2,
                                    IDB_OUTFIELD_UNREACHABLE2_HD);
  }
}
