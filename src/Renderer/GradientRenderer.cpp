// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GradientRenderer.hpp"
#include "ui/canvas/Canvas.hpp"

#if defined(EYE_CANDY) && defined(ENABLE_OPENGL)

#include "ui/canvas/opengl/VertexPointer.hpp"
#include "util/Macros.hpp"

#endif

void
DrawVerticalGradient([[maybe_unused]] Canvas &canvas, const PixelRect &rc,
                     [[maybe_unused]] Color top_color, [[maybe_unused]] Color bottom_color, [[maybe_unused]] Color fallback_color)
{
#if defined(EYE_CANDY) && defined(ENABLE_OPENGL)
  const BulkPixelPoint vertices[] = {
    rc.GetTopLeft(),
    rc.GetTopRight(),
    rc.GetBottomLeft(),
    rc.GetBottomRight(),
  };

  const ScopeVertexPointer vp(vertices);

  const Color colors[] = {
    top_color,
    top_color,
    bottom_color,
    bottom_color,
  };

  const ScopeColorPointer cp(colors);

  static_assert(ARRAY_SIZE(vertices) == ARRAY_SIZE(colors),
                "Array size mismatch");

  glDrawArrays(GL_TRIANGLE_STRIP, 0, ARRAY_SIZE(vertices));
#else
  canvas.DrawFilledRectangle(rc, fallback_color);
#endif
}
