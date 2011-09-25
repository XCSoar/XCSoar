/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Screen/Icon.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Canvas.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Texture.hpp"
#include "Screen/OpenGL/Scope.hpp"
#include "Screen/OpenGL/Compatibility.hpp"
#endif

void
MaskedIcon::load_big(unsigned id, unsigned big_id, bool center)
{
  if (Layout::ScaleEnabled()) {
    if (big_id > 0)
      bitmap.load(big_id);
    else
      bitmap.load_stretch(id, Layout::FastScale(1));
  } else
    bitmap.load(id);

#ifdef ENABLE_OPENGL
  /* postpone CalculateLayout() call, because the OpenGL surface may
     be absent now */
  size.cx = 0;
  size.cy = center;
#else
  assert(defined());

  CalculateLayout(center);
#endif
}

void
MaskedIcon::CalculateLayout(bool center)
{
  size = bitmap.get_size();
  /* left half is mask, right half is icon */
  size.cx /= 2;

  if (center) {
    origin.x = size.cx / 2;
    origin.y = size.cy / 2;
  } else {
    origin.x = 0;
    origin.y = 0;
  }
}

void
MaskedIcon::draw(Canvas &canvas, PixelScalar x, PixelScalar y) const
{
  assert(defined());

#ifdef ENABLE_OPENGL
  if (size.cx == 0)
    /* hack: do the postponed layout calcuation now */
    const_cast<MaskedIcon *>(this)->CalculateLayout((bool)size.cy);

  GLTexture &texture = *bitmap.native();

  GLEnable scope(GL_TEXTURE_2D);
  texture.bind();

  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

  GLLogicOp logic_op(GL_OR);
  texture.draw(x - origin.x, y - origin.y, size.cx, size.cy,
               0, 0, size.cx, size.cy);

  logic_op.set(GL_AND);
  texture.draw(x - origin.x, y - origin.y, size.cx, size.cy,
               size.cx, 0, size.cx, size.cy);
#else
  canvas.copy_or(x - origin.x, y - origin.y, size.cx, size.cy,
                 bitmap, 0, 0);
  canvas.copy_and(x - origin.x, y - origin.y, size.cx, size.cy,
                  bitmap, size.cx, 0);
#endif
}
