/*
Copyright_License {

  XCSoar Glide Compute5r - http://www.xcsoar.org/
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

#include "Screen/OpenGL/Shapes.hpp"
#include "Screen/OpenGL/Buffer.hpp"
#include "Screen/OpenGL/Globals.hpp"
#include "Math/FastTrig.hpp"

static GLshort circle_data[OpenGL::CIRCLE_SIZE * 2];
static GLshort small_circle_data[OpenGL::SMALL_CIRCLE_SIZE * 2];

namespace OpenGL {
  GLArrayBuffer *circle_buffer, *small_circle_buffer;
}

void
OpenGL::InitShapes()
{
  if (!OpenGL::vertex_buffer_object)
    return;

  DeinitShapes();

  assert(4096 % CIRCLE_SIZE == 0);  // implies: assert(SIZE % 2 == 0)

  GLshort *p = circle_data, *p2 = circle_data + CIRCLE_SIZE;
  for (unsigned i = 0; i < CIRCLE_SIZE / 2; ++i) {
    GLshort x = ISINETABLE[(i * (4096 / CIRCLE_SIZE) + 1024) & 0xfff];
    GLshort y = ISINETABLE[i * (4096 / CIRCLE_SIZE)];

    *p++ = x;
    *p++ = y;

    *p2++ = -x;
    *p2++ = -y;
  }

  circle_buffer = new GLArrayBuffer();
  circle_buffer->Load(GLsizeiptr(sizeof(circle_data)), circle_data);

  p = small_circle_data;
  p2 = circle_data;
  for (unsigned i = 0; i < SMALL_CIRCLE_SIZE; ++i) {
    *p++ = p2[0] >> 2;
    *p++ = p2[1] >> 2;

    p2 += 2 * CIRCLE_SIZE / SMALL_CIRCLE_SIZE;
  }

  small_circle_buffer = new GLArrayBuffer();
  small_circle_buffer->Load(GLsizeiptr(sizeof(small_circle_data)),
                            small_circle_data);
}

void
OpenGL::DeinitShapes()
{
  if (!OpenGL::vertex_buffer_object)
    return;

  delete circle_buffer;
  circle_buffer = nullptr;

  delete small_circle_buffer;
  small_circle_buffer = nullptr;
}
