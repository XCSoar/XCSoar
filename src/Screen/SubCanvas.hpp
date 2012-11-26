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

#ifndef XCSOAR_SCREEN_SUB_CANVAS_HPP
#define XCSOAR_SCREEN_SUB_CANVAS_HPP

#include "Screen/Canvas.hpp"
#include "Screen/Point.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Globals.hpp"
#include "Screen/OpenGL/System.hpp"
#endif

/**
 * A #Canvas implementation which maps into a part of an existing
 * #Canvas.
 */
class SubCanvas : public Canvas {
#ifdef ENABLE_OPENGL
  RasterPoint relative;
#endif

public:
  SubCanvas(Canvas &canvas, RasterPoint _offset, PixelSize _size)
#ifdef ENABLE_OPENGL
    :relative(_offset)
#endif
  {
#ifdef ENABLE_OPENGL
    assert(canvas.offset == OpenGL::translate);
#else
    surface = canvas.surface;
#endif
    offset = canvas.offset + _offset;
    size = _size;

#ifdef ENABLE_OPENGL
    if (relative.x != 0 || relative.y != 0) {
      OpenGL::translate += _offset;

      glPushMatrix();
#ifdef HAVE_GLES
      glTranslatex((GLfixed)relative.x << 16, (GLfixed)relative.y << 16, 0);
#else
      glTranslatef(relative.x, relative.y, 0);
#endif
    }
#endif
  }

  ~SubCanvas() {
#ifdef ENABLE_OPENGL
    assert(offset == OpenGL::translate);

    if (relative.x != 0 || relative.y != 0) {
      OpenGL::translate -= relative;

      glPopMatrix();
    }
#endif
  }
};

#endif
