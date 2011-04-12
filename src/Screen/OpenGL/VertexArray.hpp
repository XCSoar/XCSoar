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

#ifndef XCSOAR_SCREEN_OPENGL_VERTEX_ARRAY_HPP
#define XCSOAR_SCREEN_OPENGL_VERTEX_ARRAY_HPP

#include "Screen/OpenGL/Point.hpp"

template<unsigned n>
struct GLVertexArray {
  enum {
    SIZE = n,
  };

  RasterPoint v[SIZE];

  void bind() const {
    glVertexPointer(2, GL_VALUE, 0, v);
  }
};

struct GLCircleVertices : public GLVertexArray<32> {
  GLCircleVertices(GLvalue center_x, GLvalue center_y, GLvalue radius);
};

struct GLDonutVertices : public GLVertexArray<66> {
  enum {
    CIRCLE_SIZE = (SIZE - 2) / 2,
  };

  GLDonutVertices(GLvalue center_x, GLvalue center_y,
                  GLvalue radius_inner, GLvalue radius_outer);

  void bind_inner_circle() const {
    glVertexPointer(2, GL_VALUE, sizeof(v[0]) * 2, v);
  }

  void bind_outer_circle() const {
    glVertexPointer(2, GL_VALUE, sizeof(v[0]) * 2, v + 1);
  }
};

#endif
