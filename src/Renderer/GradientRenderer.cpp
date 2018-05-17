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

#include "GradientRenderer.hpp"
#include "Screen/Canvas.hpp"

#if defined(EYE_CANDY) && defined(ENABLE_OPENGL)

#include "Screen/OpenGL/VertexPointer.hpp"
#include "Util/Macros.hpp"

#endif

void
DrawVerticalGradient(Canvas &canvas, const PixelRect &rc,
                     Color top_color, Color bottom_color, Color fallback_color)
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
