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

#include "TrafficRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/TrafficLook.hpp"
#include "FLARM/Traffic.hpp"
#include "GliderLink/Traffic.hpp"
#include "Math/Screen.hpp"
#include "Util/Macros.hpp"

void
TrafficRenderer::Draw(Canvas &canvas, const TrafficLook &traffic_look,
                      const FlarmTraffic &traffic, const Angle angle,
                      const FlarmColor color, const PixelPoint pt)
{
  // Create point array that will form that arrow polygon
  BulkPixelPoint arrow[] = {
    { -4, 6 },
    { 0, -8 },
    { 4, 6 },
    { 0, 3 },
    { -4, 6 },
  };

  // Select brush depending on AlarmLevel
  switch (traffic.alarm_level) {
  case FlarmTraffic::AlarmType::LOW:
  case FlarmTraffic::AlarmType::INFO_ALERT:
    canvas.Select(traffic_look.warning_brush);
    break;
  case FlarmTraffic::AlarmType::IMPORTANT:
  case FlarmTraffic::AlarmType::URGENT:
    canvas.Select(traffic_look.alarm_brush);
    break;
  case FlarmTraffic::AlarmType::NONE:
    canvas.Select(traffic_look.safe_brush);
    break;
  }

  // Select black pen
  canvas.SelectBlackPen();

  // Rotate and shift the arrow to the right position and angle
  PolygonRotateShift(arrow, ARRAY_SIZE(arrow), pt, angle);

  // Draw the arrow
  canvas.DrawPolygon(arrow, ARRAY_SIZE(arrow));

  switch (color) {
  case FlarmColor::GREEN:
    canvas.Select(traffic_look.team_pen_green);
    break;
  case FlarmColor::BLUE:
    canvas.Select(traffic_look.team_pen_blue);
    break;
  case FlarmColor::YELLOW:
    canvas.Select(traffic_look.team_pen_yellow);
    break;
  case FlarmColor::MAGENTA:
    canvas.Select(traffic_look.team_pen_magenta);
    break;
  default:
    return;
  }

  canvas.SelectHollowBrush();
  canvas.DrawCircle(pt.x, pt.y, Layout::FastScale(11));
}



void
TrafficRenderer::Draw(Canvas &canvas, const TrafficLook &traffic_look,
                      const GliderLinkTraffic &traffic, const Angle angle, const PixelPoint pt)
{
  // Create point array that will form that arrow polygon
  BulkPixelPoint arrow[] = {
    { -4, 6 },
    { 0, -8 },
    { 4, 6 },
    { 0, 3 },
    { -4, 6 },
  };

  canvas.Select(traffic_look.safe_brush);

  // Select black pen
  if (IsDithered())
    canvas.Select(Pen(Layout::FastScale(2), COLOR_BLACK));
  else
    canvas.SelectBlackPen();

  // Rotate and shift the arrow to the right position and angle
  PolygonRotateShift(arrow, ARRAY_SIZE(arrow), pt, angle);

  // Draw the arrow
  canvas.DrawPolygon(arrow, ARRAY_SIZE(arrow));

  canvas.SelectHollowBrush();
  canvas.DrawCircle(pt.x, pt.y, Layout::FastScale(11));
}
