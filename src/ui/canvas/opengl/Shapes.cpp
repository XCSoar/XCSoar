// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Shapes.hpp"
#include "Buffer.hpp"
#include "Math/FastTrig.hpp"
#include "Math/Point2D.hpp"

#include <cassert>

namespace OpenGL {

GLArrayBuffer *circle_buffer, *small_circle_buffer;

} // namespace OpenGL

static GLArrayBuffer *
MakeCircleBuffer(unsigned n) noexcept
{
  assert(INT_ANGLE_RANGE % n == 0);

  auto buffer = new GLArrayBuffer();

  FloatPoint2D *const p0 = (FloatPoint2D *)buffer->BeginWrite(sizeof(*p0) * n);
  auto *p = p0, *p2 = p + n / 2;

  for (unsigned i = 0; i < n / 2; ++i, ++p, ++p2) {
    float x = ISINETABLE[(i * (INT_ANGLE_RANGE / n) + 1024) & INT_ANGLE_MASK] / 1024.;
    float y = ISINETABLE[i * (INT_ANGLE_RANGE / n)] / 1024.;

    p->x = x;
    p->y = y;

    p2->x = -x;
    p2->y = -y;
  }

  buffer->CommitWrite(sizeof(*p0) * n, p0);
  return buffer;
}

void
OpenGL::InitShapes() noexcept
{
  DeinitShapes();

  assert(INT_ANGLE_RANGE % CIRCLE_SIZE == 0); // implies: assert(SIZE % 2 == 0)

  circle_buffer = MakeCircleBuffer(CIRCLE_SIZE);
  small_circle_buffer = MakeCircleBuffer(SMALL_CIRCLE_SIZE);
}

void
OpenGL::DeinitShapes() noexcept
{
  delete circle_buffer;
  circle_buffer = nullptr;

  delete small_circle_buffer;
  small_circle_buffer = nullptr;
}
