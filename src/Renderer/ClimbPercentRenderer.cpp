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

#include "ClimbPercentRenderer.hpp"
#include "NMEA/CirclingInfo.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Math/Angle.hpp"
#include "Look/ClimbPercentLook.hpp"

#include <algorithm>

void
ClimbPercentRenderer::Draw(const CirclingInfo& stats, Canvas &canvas,
                           const PixelRect &rc, bool inverse)
{
  const int radius = std::min(rc.GetWidth(), rc.GetHeight()) / 2 -
                     Layout::Scale(3);
  PixelPoint center;
  center.x = (rc.left + rc.right) / 2;
  center.y = (rc.bottom + rc.top) / 2;

  if (stats.time_cruise + stats.time_circling > 0) {

    canvas.SelectNullPen();

    if (stats.noncircling_climb_percentage > 0) {
      canvas.Select(look.brush_noncircling_climb);
      canvas.DrawSegment(center, radius, -Angle::FullCircle()* stats.noncircling_climb_percentage/100, Angle::Zero());
    }

    if (stats.circling_climb_percentage > 0) {
      canvas.Select(look.brush_circling_climb);
      canvas.DrawSegment(center, radius, Angle::Zero(), Angle::FullCircle()* stats.circling_climb_percentage/100);
    }

    if (stats.circling_percentage > stats.circling_climb_percentage) {
      const Pen pen_b(Layout::ScalePenWidth(1), inverse ? COLOR_BLACK : COLOR_WHITE);
      canvas.Select(pen_b);
      canvas.Select(look.brush_circling_descent);
      canvas.DrawSegment(center, radius, Angle::FullCircle()* stats.circling_climb_percentage/100,
                         Angle::FullCircle()*stats.circling_percentage/100);
    }
  }

  const Pen pen_f(Layout::ScalePenWidth(1), inverse ? COLOR_WHITE : COLOR_BLACK);
  canvas.Select(pen_f);
  canvas.SelectHollowBrush();
  canvas.DrawCircle(center.x, center.y, radius);
}
