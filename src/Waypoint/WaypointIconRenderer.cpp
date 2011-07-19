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
#include "Look/WaypointLook.hpp"
#include "Screen/Icon.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "WaypointRenderer.hpp"
#include "WaypointRendererSettings.hpp"

gcc_pure
static const MaskedIcon &
GetWaypointIcon(const WaypointLook &look, const Waypoint &wp,
                bool small_icons, const bool in_task)
{
  if (small_icons && !in_task)
    return look.small_icon;

  switch (wp.Type) {
  case Waypoint::wtMountainTop:
    return look.mountain_top_icon;
  case Waypoint::wtBridge:
    return look.bridge_icon;
  case Waypoint::wtTunnel:
    return look.tunnel_icon;
  case Waypoint::wtTower:
    return look.tower_icon;
  case Waypoint::wtPowerPlant:
    return look.power_plant_icon;
  default:
    if (in_task) {
      return look.task_turn_point_icon;
    } else {
      return look.turn_point_icon;
    }
  }
}

static void
DrawLandableBase(Canvas &canvas, const RasterPoint& pt, bool airport,
                 const fixed radius)
{
  int iradius = iround(radius);
  if (airport)
    canvas.circle(pt.x, pt.y, iradius);
  else {
    RasterPoint diamond[4];
    diamond[0].x = pt.x + 0;
    diamond[0].y = pt.y - iradius;
    diamond[1].x = pt.x + iradius;
    diamond[1].y = pt.y + 0;
    diamond[2].x = pt.x + 0;
    diamond[2].y = pt.y + iradius;
    diamond[3].x = pt.x - iradius;
    diamond[3].y = pt.y - 0;
    canvas.polygon(diamond, sizeof(diamond)/sizeof(diamond[0]));
  }
}

static void
DrawLandableRunway(Canvas &canvas, const RasterPoint &pt,
                   const Angle &angle, fixed radius, fixed width)
{
  if (radius <= fixed_zero)
    return;

  fixed x, y;
  angle.sin_cos(x, y);
  int lx = iround(x * radius * fixed_two) & ~0x1;  // make it a even number
  int ly = iround(y * radius * fixed_two) & ~0x1;
  int wx = iround(-y * width);
  int wy = iround(x * width);

  RasterPoint runway[4];
  runway[0].x = pt.x        - (lx / 2) + (wx / 2);
  runway[0].y = pt.y        + (ly / 2) - (wy / 2);
  runway[1].x = runway[0].x            - wx;
  runway[1].y = runway[0].y            + wy;
  runway[2].x = runway[1].x + lx;
  runway[2].y = runway[1].y - ly;
  runway[3].x = runway[2].x            + wx;
  runway[3].y = runway[2].y            - wy;
  canvas.polygon(runway, sizeof(runway)/sizeof(runway[0]));
}


void
WaypointIconRenderer::DrawLandable(const Waypoint &waypoint,
                                   const RasterPoint &point,
                                   Reachability reachable)
{

  if (!settings.vector_landable_rendering) {
    const MaskedIcon *icon;

    if (reachable == ReachableTerrain)
      icon = waypoint.IsAirport()
        ? &look.airport_reachable_icon
        : &look.field_reachable_icon;
    else if (reachable == ReachableStraight)
      icon = waypoint.IsAirport()
        ? &look.airport_marginal_icon
        : &look.field_marginal_icon;
    else
      icon = waypoint.IsAirport()
        ? &look.airport_unreachable_icon
        : &look.field_unreachable_icon;

    icon->draw(canvas, point);
    return;
  }

  // SW rendering of landables
  fixed scale = fixed(Layout::SmallScale(settings.landable_rendering_scale)) /
                fixed_int_constant(150);
  fixed radius = fixed_int_constant(10) * scale;

  canvas.black_pen();
  if (settings.landable_style == wpLandableWinPilot) {
    // Render landable with reachable state
    if (reachable != Unreachable) {
      canvas.select(reachable == ReachableTerrain
                    ? look.reachable_brush
                    : look.terrain_unreachable_brush);
      DrawLandableBase(canvas, point, waypoint.IsAirport(),
                       radius + radius / fixed_two);
    }
    canvas.select(look.magenta_brush);
  } else if (settings.landable_style == wpLandableAltB) {
    if (reachable != Unreachable)
      canvas.select(reachable == ReachableTerrain
                    ? look.reachable_brush
                    : look.orange_brush);
    else
      canvas.select(look.unreachable_brush);
  } else {
    if (reachable != Unreachable)
      canvas.select(reachable == ReachableTerrain
                    ? look.reachable_brush
                    : look.terrain_unreachable_brush);
    else if (waypoint.IsAirport())
      canvas.select(look.white_brush);
    else
      canvas.select(look.light_gray_brush);
  }
  DrawLandableBase(canvas, point, waypoint.IsAirport(), radius);

  // Render runway indication
  const Runway &runway = waypoint.runway;
  if (runway.IsDirectionDefined()) {
    fixed len;
    if (settings.scale_runway_length && runway.IsLengthDefined())
      len = (radius / fixed_two) +
        (((int) runway.GetLength() - 500) / 500) * (radius / fixed_four);
    else
      len = radius;
    len += fixed_two * scale;
    Angle runwayDrawingAngle = runway.GetDirection() - screen_rotation;
    canvas.select(look.white_brush);
    DrawLandableRunway(canvas, point, runwayDrawingAngle, len,
                       fixed_int_constant(5) * scale);
  }
}

void
WaypointIconRenderer::Draw(const Waypoint &waypoint, const RasterPoint &point,
                           Reachability reachable, bool in_task)
{
  if (waypoint.IsLandable())
    DrawLandable(waypoint, point, reachable);
  else
    // non landable turnpoint
    GetWaypointIcon(look, waypoint, small_icons, in_task).draw(canvas, point);
}
