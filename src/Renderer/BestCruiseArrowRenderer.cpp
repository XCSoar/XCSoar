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

#include "BestCruiseArrowRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Look/TaskLook.hpp"
#include "Math/Angle.hpp"
#include "Math/Screen.hpp"
#include "NMEA/Derived.hpp"
#include "Util/Macros.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#endif

void
BestCruiseArrowRenderer::Draw(Canvas &canvas, const TaskLook &look,
                              const Angle screen_angle,
                              const Angle best_cruise_angle,
                              const PixelPoint pos)
{
  canvas.Select(look.best_cruise_track_pen);
  canvas.Select(look.best_cruise_track_brush);

  BulkPixelPoint arrow[] = {
    { -1, -40 },
    { -1, -62 },
    { -6, -62 },
    {  0, -70 },
    {  6, -62 },
    {  1, -62 },
    {  1, -40 },
    { -1, -40 },
  };

  PolygonRotateShift(arrow, ARRAY_SIZE(arrow), pos,
                     best_cruise_angle - screen_angle);
#ifdef ENABLE_OPENGL
  const ScopeAlphaBlend alpha_blend;
#endif
  canvas.DrawPolygon(arrow, ARRAY_SIZE(arrow));
}

void
BestCruiseArrowRenderer::Draw(Canvas &canvas, const TaskLook &look,
                              const Angle screen_angle, const PixelPoint pos,
                              const DerivedInfo &calculated)
{
  if (calculated.turn_mode == CirclingMode::CLIMB ||
      !calculated.task_stats.task_valid)
    return;

  const GlideResult &solution =
      calculated.task_stats.current_leg.solution_remaining;

  if (!solution.IsOk() || solution.vector.distance < 0.01)
    return;

  BestCruiseArrowRenderer::Draw(canvas, look, screen_angle,
                                solution.cruise_track_bearing, pos);
}
