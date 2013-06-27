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

#ifndef XCSOAR_SCREEN_POINT_HPP
#define XCSOAR_SCREEN_POINT_HPP

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Point.hpp"
#elif defined(USE_MEMORY_CANVAS)
#include "Screen/Memory/Point.hpp"
#elif defined(USE_GDI)
#include "Screen/GDI/Point.hpp"
#else
#error No Point implementation
#endif

#include "Compiler.h"

#include <stdlib.h>

/**
 * Calculates the "manhattan distance" or "taxicab distance".
 */
static inline unsigned
manhattan_distance(RasterPoint a, RasterPoint b)
{
  return abs(a.x - b.x) + abs(a.y - b.y);
}

gcc_pure
static inline bool
OverlapsRect(const PixelRect &a, const PixelRect &b)
{
  return a.left < b.right && b.left <= a.right &&
    a.top <= b.bottom && b.top <= a.bottom;
}

static inline void
SetRect(PixelRect &rc, PixelScalar left, PixelScalar top,
        PixelScalar right, PixelScalar bottom)
{
#ifdef USE_GDI
  SetRect(&rc, left, top, right, bottom);
#else
  rc.left = left;
  rc.top = top;
  rc.right = right;
  rc.bottom = bottom;
#endif
}

#endif
