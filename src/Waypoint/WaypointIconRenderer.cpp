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

#include "WaypointIconRenderer.hpp"
#include "Screen/Icon.hpp"
#include "Screen/Graphics.hpp"
#include "WaypointRenderer.hpp"

gcc_pure
static const MaskedIcon &
GetWaypointIcon(const Waypoint &wp, bool small_icons, const bool in_task)
{
  if (small_icons && !in_task)
    return Graphics::SmallIcon;

  switch (wp.Type) {
  case Waypoint::wtMountainTop:
    return Graphics::MountainTopIcon;
  case Waypoint::wtBridge:
    return Graphics::BridgeIcon;
  case Waypoint::wtTunnel:
    return Graphics::TunnelIcon;
  case Waypoint::wtTower:
    return Graphics::TowerIcon;
  case Waypoint::wtPowerPlant:
    return Graphics::PowerPlantIcon;
  default:
    if (in_task) {
      return Graphics::TaskTurnPointIcon;
    } else {
      return Graphics::TurnPointIcon;
    }
  }
}

void
WaypointIconRenderer::Draw(const Waypoint &waypoint, RasterPoint point,
                           Reachability reachable, bool in_task)
{
  if (waypoint.IsLandable())
    WaypointRenderer::DrawLandableSymbol(canvas, point,
      (WaypointRenderer::Reachability)reachable, waypoint, screen_rotation);
  else
    // non landable turnpoint
    GetWaypointIcon(waypoint, small_icons, in_task).draw(canvas, point);
}
