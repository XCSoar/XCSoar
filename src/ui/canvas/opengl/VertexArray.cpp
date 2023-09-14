// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VertexArray.hpp"
#include "Math/FastTrig.hpp"

GLDonutVertices::GLDonutVertices(GLvalue center_x, GLvalue center_y,
                                 GLvalue radius_inner,
                                 GLvalue radius_outer) noexcept
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
