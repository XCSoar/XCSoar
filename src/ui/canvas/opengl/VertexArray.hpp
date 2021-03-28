/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "VertexPointer.hpp"
#include "ui/dim/BulkPoint.hpp"
#include "util/Compiler.h"

template<unsigned n>
struct GLVertexArray {
  static constexpr unsigned SIZE = n;

  BulkPixelPoint v[SIZE];

  void Bind(ScopeVertexPointer &vp) const noexcept {
    vp.Update(v);
  }
};

struct GLDonutVertices : public GLVertexArray<66> {
  static constexpr unsigned CIRCLE_SIZE = (SIZE - 2) / 2;
  static constexpr unsigned MAX_ANGLE = CIRCLE_SIZE * 2u;

  static constexpr unsigned FixAngle(unsigned angle) noexcept {
    return angle & ((CIRCLE_SIZE - 1u) * 2u);
  }

  static constexpr unsigned PreviousAngle(unsigned angle) noexcept {
    return FixAngle(angle - 2u);
  }

  static constexpr unsigned NextAngle(unsigned angle) noexcept {
    return FixAngle(angle + 2u);
  }

  static constexpr unsigned ImportAngle(unsigned other_angle,
                                        unsigned other_max) noexcept {
    return ((other_angle * CIRCLE_SIZE / other_max) % CIRCLE_SIZE) * 2u;
  }

  GLDonutVertices(GLvalue center_x, GLvalue center_y,
                  GLvalue radius_inner, GLvalue radius_outer) noexcept;

  void BindInnerCircle(ScopeVertexPointer &vp) const noexcept {
    vp.Update(GL_VALUE, sizeof(v[0]) * 2, v);
  }

  void BindOuterCircle(ScopeVertexPointer &vp) const noexcept {
    vp.Update(GL_VALUE, sizeof(v[0]) * 2, v + 1);
  }
};

#endif
