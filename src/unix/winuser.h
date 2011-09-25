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

#ifndef WINUSER_H
#define WINUSER_H

#include "Screen/Point.hpp"

#include <windef.h>

enum {
  DT_EXPANDTABS = 0x1,
  DT_LEFT = 0x2,
  DT_NOCLIP = 0x4,
  DT_WORDBREAK = 0x8,
  DT_CENTER = 0x20,
  DT_NOPREFIX = 0x40,
  DT_VCENTER = 0x80,
  DT_RIGHT = 0x100,
  DT_CALCRECT = 0x400,
};

static inline void
SetRect(PixelRect *rc, PixelScalar left, PixelScalar top,
        PixelScalar right, PixelScalar bottom)
{
  rc->left = left;
  rc->top = top;
  rc->right = right;
  rc->bottom = bottom;
}

static inline void
SetRectEmpty(PixelRect *rc)
{
  rc->left = 0;
  rc->top = 0;
  rc->right = 0;
  rc->bottom = 0;
}

static inline void
CopyRect(PixelRect *dest, const PixelRect *src)
{
  *dest = *src;
}

static inline void
InflateRect(PixelRect *rc, PixelScalar dx, PixelScalar dy)
{
  rc->left -= dx;
  rc->top -= dy;
  rc->right += dx;
  rc->bottom += dy;
}

static inline void
OffsetRect(PixelRect *rc, PixelScalar dx, PixelScalar dy)
{
  rc->left += dx;
  rc->top += dy;
  rc->right += dx;
  rc->bottom += dy;
}

static inline bool
PtInRect(const PixelRect *rc, const RasterPoint &pt)
{
  return pt.x >= rc->left && pt.x < rc->right &&
    pt.y >= rc->top && pt.y < rc->bottom;
}

static inline bool
EqualRect(const PixelRect *a, const PixelRect *b)
{
  return a->left == b->left && a->top == b->top &&
    a->right == b->right && a->bottom == b->bottom;
}

static inline bool
IntersectRect(PixelRect *dest, const PixelRect *a, const PixelRect *b)
{
  dest->left = a->left < b->left ? a->left : b->left;
  dest->top = a->top < b->top ? a->top : b->top;
  dest->right = a->right > b->right ? a->right : b->right;
  dest->bottom = a->bottom > b->bottom ? a->bottom : b->bottom;

  return dest->left < dest->right && dest->top < dest->bottom;
}

#endif
