/*
 Copyright_License {

 XCSoar Glide Computer - http://www.xcsoar.org/
 Copyright (C) 2000-2012 The XCSoar Project
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
#include "Math/Screen.hpp"

void
TrafficRenderer::Draw(Canvas &canvas, const TrafficLook &traffic_look,
                      const FlarmTraffic &traffic, const Angle angle,
                      const FlarmFriends::Color color, const RasterPoint pt)
{
  // Create point array that will form that arrow polygon
  RasterPoint Arrow[5];

  // Fill the Arrow array with a normal arrow pointing north
  Arrow[0].x = -4;
  Arrow[0].y = 6;
  Arrow[1].x = 0;
  Arrow[1].y = -8;
  Arrow[2].x = 4;
  Arrow[2].y = 6;
  Arrow[3].x = 0;
  Arrow[3].y = 3;
  Arrow[4].x = -4;
  Arrow[4].y = 6;

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
  PolygonRotateShift(Arrow, 5, pt.x, pt.y, angle);

  // Draw the arrow
  canvas.DrawPolygon(Arrow, 5);

  switch (color) {
  case FlarmFriends::Color::GREEN:
    canvas.Select(traffic_look.team_pen_green);
    break;
  case FlarmFriends::Color::BLUE:
    canvas.Select(traffic_look.team_pen_blue);
    break;
  case FlarmFriends::Color::YELLOW:
    canvas.Select(traffic_look.team_pen_yellow);
    break;
  case FlarmFriends::Color::MAGENTA:
    canvas.Select(traffic_look.team_pen_magenta);
    break;
  default:
    return;
  }

  canvas.SelectHollowBrush();
  canvas.DrawCircle(pt.x, pt.y, Layout::FastScale(11));
}
