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

#include "TaskProgressRenderer.hpp"
#include "Look/TaskLook.hpp"
#include "Engine/Task/Stats/TaskSummary.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Math/Angle.hpp"

#include <algorithm>

void 
TaskProgressRenderer::Draw(const TaskSummary& summary, Canvas &canvas,
                           const PixelRect &rc, bool inverse)
{
  const int radius = std::min(rc.GetWidth(), rc.GetHeight()) / 2 -
                     Layout::Scale(3);
  PixelPoint center;
  center.x = (rc.left + rc.right) / 2;
  center.y = (rc.bottom + rc.top) / 2;

  const Angle sweep = Angle::FullCircle() * 0.97;

  if (summary.p_remaining < 0.99) {
    canvas.Select(look.hbGray);
    canvas.SelectNullPen();
    canvas.DrawSegment(center, radius, Angle::Zero(),
                       sweep * (1 -  summary.p_remaining));
  }

  const Pen pen_f(Layout::ScalePenWidth(1), inverse ? COLOR_WHITE : COLOR_BLACK);
  canvas.Select(pen_f);
  canvas.SelectHollowBrush();
  canvas.DrawCircle(center.x, center.y, radius);

  unsigned i = 0;
  for (auto it = summary.pts.begin(); it != summary.pts.end(); ++it, ++i) {
    Angle a = sweep * it->p;
    int x = center.x + (int)(radius * a.fastsine());
    int y = center.y - (int)(radius * a.fastcosine());
    int w;
    if (i == summary.active) {
      if (it->achieved)
        canvas.Select(look.hbGreen);
      else
        canvas.Select(look.hbOrange);

      w = Layout::Scale(3);
    } else if (i < summary.active) {
      if (it->achieved)
        canvas.Select(look.hbGreen);
      else
        canvas.Select(look.hbNotReachableTerrain);

      w = Layout::Scale(2);
    } else {
      if (it->achieved)
        canvas.Select(look.hbGreen);
      else
        canvas.Select(look.hbLightGray);

      w = Layout::Scale(1);
    }
    
    canvas.Rectangle(x - w, y - w, x + w, y + w);
  }
}
