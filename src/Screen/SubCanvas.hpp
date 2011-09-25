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

#ifndef XCSOAR_SCREEN_SUB_CANVAS_HPP
#define XCSOAR_SCREEN_SUB_CANVAS_HPP

#include "Screen/Canvas.hpp"
#include "Screen/Point.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Globals.hpp"
#ifdef ANDROID
#include <GLES/gl.h>
#else
#include <SDL/SDL_opengl.h>
#endif
#endif

/**
 * A #Canvas implementation which maps into a part of an existing
 * #Canvas.
 */
class SubCanvas : public Canvas {
#ifdef ENABLE_OPENGL
  GLvalue relative_x, relative_y;
#endif

public:
  SubCanvas(Canvas &canvas, PixelScalar _x, PixelScalar _y,
            UPixelScalar _width, UPixelScalar _height)
#ifdef ENABLE_OPENGL
    :relative_x(_x), relative_y(_y)
#endif
  {
#ifdef ENABLE_OPENGL
    assert(canvas.x_offset == OpenGL::translate_x);
    assert(canvas.y_offset == OpenGL::translate_y);
    OpenGL::translate_x += _x;
    OpenGL::translate_y += _y;
#else
    surface = canvas.surface;
#endif
    x_offset = canvas.x_offset + _x;
    y_offset = canvas.y_offset + _y;
    width = _width;
    height = _height;

#ifdef ENABLE_OPENGL
    glPushMatrix();
#ifdef ANDROID
    glTranslatex((GLfixed)relative_x << 16, (GLfixed)relative_y << 16, 0);
#else
    glTranslatef(relative_x, relative_y, 0);
#endif
#endif
  }

  ~SubCanvas() {
#ifdef ENABLE_OPENGL
    assert(x_offset == OpenGL::translate_x);
    assert(y_offset == OpenGL::translate_y);

    OpenGL::translate_x -= relative_x;
    OpenGL::translate_y -= relative_y;

    glPopMatrix();
#endif
  }
};

#endif
