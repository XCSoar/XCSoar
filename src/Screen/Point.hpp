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

#ifndef XCSOAR_SCREEN_POINT_HPP
#define XCSOAR_SCREEN_POINT_HPP

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Point.hpp"
#elif defined(ENABLE_SDL)
#include "Screen/SDL/Point.hpp"
#else
#include "Screen/GDI/Point.hpp"
#endif

#include "Compiler.h"

#include <stdlib.h>

#ifdef USE_GDI
#include <windows.h>
#endif

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
IsPointInRect(const PixelRect &rc, const RasterPoint &pt)
{
#ifdef USE_GDI
  return PtInRect(&rc, pt);
#else
  return pt.x >= rc.left && pt.x < rc.right &&
    pt.y >= rc.top && pt.y < rc.bottom;
#endif
}

gcc_pure
static inline bool
OverlapsRect(const PixelRect &a, const PixelRect &b)
{
  return a.left < b.right && b.left <= a.right &&
    a.top <= b.bottom && b.top <= a.bottom;
}

gcc_const
static inline PixelSize
GetPixelRectSize(const PixelRect rc)
{
  return { PixelScalar(rc.right - rc.left),
      PixelScalar(rc.bottom - rc.top) };
}

static inline void
ClearRect(PixelRect &rc)
{
#ifdef USE_GDI
  SetRectEmpty(&rc);
#else
  rc.left = 0;
  rc.top = 0;
  rc.right = 0;
  rc.bottom = 0;
#endif
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

static inline void
MoveRect(PixelRect &rc, int dx, int dy)
{
#ifdef USE_GDI
  OffsetRect(&rc, dx, dy);
#else
  rc.left += dx;
  rc.top += dy;
  rc.right += dx;
  rc.bottom += dy;
#endif
}

static inline void
GrowRect(PixelRect &rc, int dx, int dy)
{
#ifdef USE_GDI
  InflateRect(&rc, dx, dy);
#else
  rc.left -= dx;
  rc.top -= dy;
  rc.right += dx;
  rc.bottom += dy;
#endif
}

#endif
