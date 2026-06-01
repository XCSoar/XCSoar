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

static constexpr struct {
  int x, y;
} template_arrow[] = {
  { -1, -40 },
  { -1, -62 },
  { -6, -62 },
  {  0, -70 },
  {  6, -62 },
  {  1, -62 },
  {  1, -40 },
};

static_assert(ARRAY_SIZE(template_arrow) == BestCruiseArrowRenderer::arrow_size,
              "template must match arrow_size");

unsigned
BestCruiseArrowRenderer::GetScale() noexcept
{
  return Layout::Scale(100U);
}

void
BestCruiseArrowRenderer::Build(BulkPixelPoint *dest, int y_offset) noexcept
{
  for (unsigned i = 0; i < arrow_size; ++i) {
    dest[i].x = template_arrow[i].x;
    dest[i].y = template_arrow[i].y + y_offset;
  }
}

int
BestCruiseArrowRenderer::YOffsetForRadius(unsigned radius_from_center,
                                          int scale) noexcept
{
  return -center_y - int(radius_from_center * 100 / scale);
}

void
BestCruiseArrowRenderer::Draw(Canvas &canvas, const TaskLook &look,
                              const Angle screen_angle,
                              const Angle best_cruise_angle,
                              const PixelPoint pos)
{
  canvas.Select(look.best_cruise_track_pen);
  canvas.Select(look.best_cruise_track_brush);

  BulkPixelPoint arrow[arrow_size];
  Build(arrow);

  const unsigned scale = GetScale();
  PolygonRotateShift(arrow, pos, best_cruise_angle - screen_angle, scale);
#ifdef ENABLE_OPENGL
  const ScopeAlphaBlend alpha_blend;
#endif
  canvas.DrawPolygon(arrow, arrow_size);
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

  Draw(canvas, look, screen_angle, solution.cruise_track_bearing, pos);
}
