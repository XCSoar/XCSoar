// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CompassRenderer.hpp"
#include "Look/MapLook.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Math/Angle.hpp"
#include "Math/Screen.hpp"
#include "util/Macros.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scope.hpp"
#endif

void
CompassRenderer::Draw(Canvas &canvas, const Angle screen_angle,
                      const PixelRect rc) noexcept
{
  PixelPoint pos(rc.right - Layout::Scale(19),
                 Layout::Scale(19) + rc.top);
  Draw(canvas, screen_angle, pos);
}

void
CompassRenderer::Draw(Canvas &canvas, const Angle screen_angle,
                      const PixelPoint pos) noexcept
{
  BulkPixelPoint arrow[] = { { 0, -13 }, { -6, 10 }, { 0, 4 }, { 6, 10 } };

  canvas.Select(look.compass_pen);
  canvas.Select(look.compass_brush);

#ifdef ENABLE_OPENGL
  const ScopeAlphaBlend alpha_blend;
#endif

  // North arrow
  PolygonRotateShift({arrow, ARRAY_SIZE(arrow)}, pos, -screen_angle,
                     Layout::Scale(100U));
  canvas.DrawPolygon(arrow, ARRAY_SIZE(arrow));

  canvas.Select(look.compass_triangle_pen);
  canvas.Select(look.compass_triangle_brush);

  BulkPixelPoint black_triangle[] = { { 0, -13 }, { 6, 10}, { 0, 4} };
  PolygonRotateShift(black_triangle, pos, -screen_angle,
                     Layout::Scale(100U));
  canvas.DrawPolygon(black_triangle, ARRAY_SIZE(black_triangle));
}
