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

#ifndef XCSOAR_SCREEN_GDI_POINT_HPP
#define XCSOAR_SCREEN_GDI_POINT_HPP

#include <windef.h>

typedef LONG PixelScalar;
typedef ULONG UPixelScalar;

struct RasterPoint : public tagPOINT {
  RasterPoint() = default;

  constexpr RasterPoint(PixelScalar _x, PixelScalar _y)
    :tagPOINT({_x, _y}) {}
};

static_assert(sizeof(RasterPoint) == sizeof(POINT), "not same size");

struct PixelRect : public tagRECT {
  PixelRect() = default;

  constexpr PixelRect(PixelScalar _left, PixelScalar _top,
                      PixelScalar _right, PixelScalar _bottom)
    :tagRECT({_left, _top, _right, _bottom}) {}
};

static_assert(sizeof(PixelRect) == sizeof(RECT), "not same size");

struct PixelSize : public tagSIZE {
  PixelSize() = default;

#ifndef __WINE__
  constexpr PixelSize(PixelScalar _width, PixelScalar _height)
    :tagSIZE({_width, _height}) {}
#endif

  constexpr PixelSize(int _width, int _height)
    :tagSIZE({_width, _height}) {}

  constexpr PixelSize(unsigned _width, unsigned _height)
    :tagSIZE({PixelScalar(_width), PixelScalar(_height)}) {}

  bool operator==(const PixelSize &other) const {
    return cx == other.cx && cy == other.cy;
  }

  bool operator!=(const PixelSize &other) const {
    return !(*this == other);
  }
};

static_assert(sizeof(PixelSize) == sizeof(SIZE), "not same size");

#endif
