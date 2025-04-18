// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BestCruiseArrowRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/TaskLook.hpp"
#include "Math/Angle.hpp"
#include "Math/Screen.hpp"
#include "NMEA/Derived.hpp"
#include "util/Macros.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
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
  };

  PolygonRotateShift(arrow, pos,
                     best_cruise_angle - screen_angle,
                     Layout::Scale(100U));
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
