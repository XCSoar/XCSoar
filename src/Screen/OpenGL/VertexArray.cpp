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

#include "Screen/OpenGL/VertexArray.hpp"
#include "Math/FastTrig.hpp"

GLDonutVertices::GLDonutVertices(GLvalue center_x, GLvalue center_y,
                                 GLvalue radius_inner, GLvalue radius_outer)
{
  static_assert(INT_ANGLE_RANGE % CIRCLE_SIZE == 0, "Wrong CIRCLE_SIZE");
  auto *p = v, *p2 = v + CIRCLE_SIZE;

  for (unsigned i = 0; i < CIRCLE_SIZE/2; ++i) {
    int cos = ISINETABLE[(i * (INT_ANGLE_RANGE / CIRCLE_SIZE) + 1024) & INT_ANGLE_MASK];
    int sin = ISINETABLE[i * (INT_ANGLE_RANGE / CIRCLE_SIZE)];

    int offset_x = cos * (int)radius_inner / 1024.;
    int offset_y = sin * (int)radius_inner / 1024.;
    p->x = center_x + offset_x;
    p->y = center_y + offset_y;
    ++p;
    p2->x = center_x - offset_x;
    p2->y = center_y - offset_y;
    ++p2;

    offset_x = cos * (int)radius_outer / 1024.;
    offset_y = sin * (int)radius_outer / 1024.;
    p->x = center_x + offset_x;
    p->y = center_y + offset_y;
    ++p;
    p2->x = center_x - offset_x;
    p2->y = center_y - offset_y;
    ++p2;
  }
  p2->x = v[0].x;
  p2->y = v[0].y;
  ++p2;
  p2->x = v[1].x;
  p2->y = v[1].y;
}
