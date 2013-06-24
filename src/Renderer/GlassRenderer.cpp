/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "GlassRenderer.hpp"
#include "Screen/Canvas.hpp"

#if defined(EYE_CANDY) && defined(ENABLE_OPENGL)

#include "Screen/OpenGL/Scissor.hpp"
#include "Util/Macros.hpp"

#endif

void
DrawGlassBackground(Canvas &canvas, const PixelRect &rc, Color color)
{
  canvas.DrawFilledRectangle(rc, color);

#if defined(EYE_CANDY) && defined(ENABLE_OPENGL)
  if (color != COLOR_WHITE)
    /* apply only to white background for now */
    return;

  const GLCanvasScissor scissor(rc);

  const Color shadow = color.Shadow();

  const RasterPoint center = rc.GetCenter();
  const int size = std::min(rc.right - rc.left, rc.bottom - rc.top) / 4;

  const RasterPoint vertices[] = {
    { center.x + 1024, center.y - 1024 },
    { center.x + 1024 + size, center.y - 1024 + size },
    { center.x - 1024, center.y + 1024 },
    { center.x - 1024 + size, center.y + 1024 + size },
  };

  glVertexPointer(2, GL_VALUE, 0, vertices);

  const Color colors[] = {
    shadow, color,
    shadow, color,
  };

  glEnableClientState(GL_COLOR_ARRAY);

#ifdef HAVE_GLES
  glColorPointer(4, GL_FIXED, 0, colors);
#else
  glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);
#endif

  static_assert(ARRAY_SIZE(vertices) == ARRAY_SIZE(colors),
                "Array size mismatch");

  glDrawArrays(GL_TRIANGLE_STRIP, 0, ARRAY_SIZE(vertices));

  glDisableClientState(GL_COLOR_ARRAY);
#endif
}
