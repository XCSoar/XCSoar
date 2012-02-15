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

#include "FinalGlideBarRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Screen/TextInBox.hpp"
#include "NMEA/Derived.hpp"
#include "Look/FinalGlideBarLook.hpp"
#include "Look/TaskLook.hpp"
#include "Units/UserUnitsFormatter.hpp"
#include "Util/Macros.hpp"

void
FinalGlideBarRenderer::Draw(Canvas &canvas, const PixelRect &rc,
                            const DerivedInfo &calculated) const
{
  RasterPoint GlideBar[6] = {
      { 0, 0 }, { 9, -9 }, { 18, 0 }, { 18, 0 }, { 9, 0 }, { 0, 0 }
  };
  RasterPoint GlideBar0[6] = {
      { 0, 0 }, { 9, -9 }, { 18, 0 }, { 18, 0 }, { 9, 0 }, { 0, 0 }
  };

  TCHAR Value[10];

  if (!calculated.task_stats.task_valid ||
      !calculated.task_stats.total.solution_remaining.IsOk() ||
      !calculated.task_stats.total.solution_mc0.IsDefined())
    return;

  const int y0 = (rc.bottom + rc.top) / 2;

  // 480 meters is it's size. Will be divided by 8 to fit screen resolution.
  int altitude_difference = ((int)calculated.task_stats.total.solution_remaining.altitude_difference);
  int altitude_difference0 = ((int)calculated.task_stats.total.solution_mc0.altitude_difference);
  // TODO feature: should be an angle if in final glide mode

  if (altitude_difference > 480)
    altitude_difference = 480;
  if (altitude_difference < -480)
    altitude_difference = -480;

  // 60 units is size, 480 meters div by 8 means 60.
  int Offset = altitude_difference / 8;
  
  Offset = Layout::Scale(Offset);
  if (altitude_difference < 0)
    GlideBar[1].y = Layout::Scale(9);

  if (altitude_difference0 > 480)
    altitude_difference0 = 480;
  if (altitude_difference0 < -480)
    altitude_difference0 = -480;

  // 60 units is size, 480 meters div by 8 means 60.
  int Offset0 = altitude_difference0 / 8;
  
  Offset0 = Layout::Scale(Offset0);
  if (altitude_difference0 < 0)
    GlideBar0[1].y = Layout::Scale(9);

  for (unsigned i = 0; i < 6; i++) {
    GlideBar[i].y += y0;
    GlideBar[i].x = Layout::Scale(GlideBar[i].x) + rc.left;
  }

  GlideBar[0].y -= Offset;
  GlideBar[1].y -= Offset;
  GlideBar[2].y -= Offset;

  for (unsigned i = 0; i < 6; i++) {
    GlideBar0[i].y += y0;
    GlideBar0[i].x = Layout::Scale(GlideBar0[i].x) + rc.left;
  }

  GlideBar0[0].y -= Offset0;
  GlideBar0[1].y -= Offset0;
  GlideBar0[2].y -= Offset0;

  if ((altitude_difference < 0) && (altitude_difference0 < 0)) {
    // both below
    if (altitude_difference0 != altitude_difference) {
      PixelScalar dy = (GlideBar0[0].y - GlideBar[0].y);
      // keep some significant size of solid arrow.
      dy = max(Layout::Scale(6), dy);

      GlideBar[3].y = GlideBar0[0].y - dy;
      GlideBar[4].y = GlideBar0[1].y - dy;
      GlideBar[5].y = GlideBar0[2].y - dy;

      GlideBar0[0].y = GlideBar[3].y;
      GlideBar0[1].y = GlideBar[4].y;
      GlideBar0[2].y = GlideBar[5].y;
    } else {
      // don't draw hollow arrow if altitude_difference equals altitude_difference0
      altitude_difference0 = 0;
    }
  } else if ((altitude_difference > 0) && (altitude_difference0 > 0)) {
    // both above
    GlideBar0[3].y = GlideBar[0].y;
    GlideBar0[4].y = GlideBar[1].y;
    GlideBar0[5].y = GlideBar[2].y;

    // don't draw hollow arrow Offset0 - Offset is small
    if (abs(Offset0 - Offset) < Layout::Scale(4))
      altitude_difference = altitude_difference0;
  }

  // draw actual glide bar
  if (altitude_difference <= 0) {
    if (calculated.common_stats.landable_reachable) {
      canvas.Select(look.hpFinalGlideBelowLandable);
      canvas.Select(look.hbFinalGlideBelowLandable);
    } else {
      canvas.Select(look.hpFinalGlideBelow);
      canvas.Select(look.hbFinalGlideBelow);
    }
  } else {
    canvas.Select(look.hpFinalGlideAbove);
    canvas.Select(look.hbFinalGlideAbove);
  }
  canvas.polygon(GlideBar, 6);

  // draw glide bar at mc 0
  if (altitude_difference0 <= 0) {
    if (calculated.common_stats.landable_reachable) {
      canvas.Select(look.hpFinalGlideBelowLandable);
      canvas.SelectHollowBrush();
    } else {
      canvas.Select(look.hpFinalGlideBelow);
      canvas.SelectHollowBrush();
    }
  } else {
    canvas.Select(look.hpFinalGlideAbove);
    canvas.SelectHollowBrush();
  }

  if (altitude_difference != altitude_difference0)
    canvas.polygon(GlideBar0, 6);

  // draw cross (x) on final glide bar if unreachable at current Mc
  // or above final glide but impeded by obstacle
  int cross_sign = 0;

  if (!calculated.task_stats.total.IsAchievable())
    cross_sign = 1;
  if (calculated.terrain_warning && (altitude_difference>0))
    cross_sign = -1;

  if (cross_sign != 0) {
    canvas.Select(task_look.bearing_pen);
    canvas.line(Layout::Scale(9 - 5), y0 + cross_sign * Layout::Scale(9 - 5),
                Layout::Scale(9 + 5), y0 + cross_sign * Layout::Scale(9 + 5));
    canvas.line(Layout::Scale(9 - 5), y0 + cross_sign * Layout::Scale(9 + 5),
                Layout::Scale(9 + 5), y0 + cross_sign * Layout::Scale(9 - 5));
  }

  Units::FormatUserAltitude(calculated.task_stats.total.solution_remaining.altitude_difference,
                            Value, false);

  if (altitude_difference >= 0)
    Offset = GlideBar[2].y + Offset + Layout::Scale(5);
  else if (altitude_difference0 > 0)
    Offset = GlideBar0[1].y - Layout::Scale(15);
  else
    Offset = GlideBar[2].y + Offset - Layout::Scale(15);

  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundColor(COLOR_WHITE);

  TextInBoxMode style;
  style.mode = RM_ROUNDED_BLACK;
  style.bold = true;
  style.move_in_view = true;
  TextInBox(canvas, Value, 0, (int)Offset, style, rc);
}
