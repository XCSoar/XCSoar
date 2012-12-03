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

#ifndef XCSOAR_SCREEN_CUSTOM_POINT_HPP
#define XCSOAR_SCREEN_CUSTOM_POINT_HPP

struct PixelSize;

struct RasterPoint {
  /**
   * Type to be used by vector math, where a range of
   * max(GLvalue)*max(GLvalue) is needed.
   */
  typedef int SquareType;
  PixelScalar x, y;

  RasterPoint() = default;

  constexpr RasterPoint(int _x, int _y)
    :x(_x), y(_y) {}

  bool operator==(const RasterPoint &other) const {
    return x == other.x && y == other.y;
  }

  bool operator!=(const RasterPoint &other) const {
    return !(*this == other);
  }

  constexpr RasterPoint operator+(RasterPoint other) const {
    return { x + other.x, y + other.y };
  }

  RasterPoint &operator+=(RasterPoint other) {
    x += other.x;
    y += other.y;
    return *this;
  }

  RasterPoint &operator-=(RasterPoint other) {
    x -= other.x;
    y -= other.y;
    return *this;
  }

  constexpr RasterPoint operator+(PixelSize size) const;
};

struct PixelSize {
  PixelScalar cx, cy;

  PixelSize() = default;

  constexpr PixelSize(int _width, int _height)
    :cx(_width), cy(_height) {}

  constexpr PixelSize(unsigned _width, unsigned _height)
    :cx(_width), cy(_height) {}

  bool operator==(const PixelSize &other) const {
    return cx == other.cx && cy == other.cy;
  }

  bool operator!=(const PixelSize &other) const {
    return !(*this == other);
  }
};

inline constexpr RasterPoint
RasterPoint::operator+(PixelSize size) const {
  return { x + size.cx, y + size.cy };
}

struct PixelRect {
  PixelScalar left, top, right, bottom;

  PixelRect() = default;

  constexpr PixelRect(int _left, int _top, int _right, int _bottom)
    :left(_left), top(_top), right(_right), bottom(_bottom) {}

  constexpr PixelRect(RasterPoint origin, PixelSize size)
    :left(origin.x), top(origin.y),
     right(origin.x + size.cx), bottom(origin.y + size.cy) {}

  explicit constexpr PixelRect(PixelSize size)
    :left(0), top(0), right(size.cx), bottom(size.cy) {}

  void SetEmpty() {
    left = top = right = bottom = 0;
  }

  void Offset(int dx, int dy) {
    left += dx;
    top += dy;
    right += dx;
    bottom += dy;
  }

  void Grow(int dx, int dy) {
    left -= dx;
    top -= dy;
    right += dx;
    bottom += dy;
  }

  void Grow(int d) {
    Grow(d, d);
  }

  constexpr RasterPoint GetOrigin() const {
    return { left, top };
  }

  constexpr PixelSize GetSize() const {
    return { right - left, bottom - top };
  }

  constexpr RasterPoint GetTopLeft() const {
    return { left, top };
  }

  constexpr RasterPoint GetTopRight() const {
    return { right, top };
  }

  constexpr RasterPoint GetBottomLeft() const {
    return { left, bottom };
  }

  constexpr RasterPoint GetBottomRight() const {
    return { right, bottom };
  }

  constexpr bool IsInside(RasterPoint pt) const {
    return pt.x >= left && pt.x < right && pt.y >= top && pt.y < bottom;
  }
};

#endif
