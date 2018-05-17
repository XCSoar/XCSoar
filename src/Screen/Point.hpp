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

#ifndef XCSOAR_SCREEN_POINT_HPP
#define XCSOAR_SCREEN_POINT_HPP

#include "Math/Point2D.hpp"

#ifdef USE_WINUSER
#include <windows.h>
#endif

struct PixelPoint : IntPoint2D {
  PixelPoint() = default;

  template<typename... Args>
  constexpr PixelPoint(Args&&... args)
    :IntPoint2D(args...) {}
};

struct PixelSize {
  int cx, cy;

  PixelSize() = default;

  constexpr PixelSize(int _width, int _height)
    :cx(_width), cy(_height) {}

  constexpr PixelSize(unsigned _width, unsigned _height)
    :cx(_width), cy(_height) {}

  constexpr PixelSize(long _width, long _height)
    :cx(_width), cy(_height) {}

  bool operator==(const PixelSize &other) const {
    return cx == other.cx && cy == other.cy;
  }

  bool operator!=(const PixelSize &other) const {
    return !(*this == other);
  }
};

inline constexpr PixelPoint
operator+(PixelPoint p, PixelSize size)
{
  return { p.x + size.cx, p.y + size.cy };
}

/**
 * @brief PixelRect structure and operations
 *
 * Provides support for creating and manipulating PixelRect structures
 *
 * @note This structure follows the GDI convention of the {right, bottom} coordinates being
 * immediately outside the rectangle being specified.
 */
struct PixelRect {
  int left, top, right, bottom;

  PixelRect() = default;

  constexpr PixelRect(int _left, int _top, int _right, int _bottom)
    :left(_left), top(_top), right(_right), bottom(_bottom) {}

  constexpr PixelRect(PixelPoint origin, PixelSize size)
    :left(origin.x), top(origin.y),
     right(origin.x + size.cx), bottom(origin.y + size.cy) {}

  explicit constexpr PixelRect(PixelSize size)
    :left(0), top(0), right(size.cx), bottom(size.cy) {}

  bool IsEmpty() {
    return left >= right || top >= bottom;
  }

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

  constexpr PixelPoint GetOrigin() const {
    return { left, top };
  }

  constexpr PixelSize GetSize() const {
    return { right - left, bottom - top };
  }

  constexpr PixelPoint GetCenter() const {
    return { (left + right) / 2, (top + bottom) / 2 };
  }

  constexpr PixelPoint GetTopLeft() const {
    return { left, top };
  }

  constexpr PixelPoint GetTopRight() const {
    return { right, top };
  }

  constexpr PixelPoint GetBottomLeft() const {
    return { left, bottom };
  }

  constexpr PixelPoint GetBottomRight() const {
    return { right, bottom };
  }

  constexpr unsigned GetWidth() const {
    return right - left;
  }

  constexpr unsigned GetHeight() const {
    return bottom - top;
  }

  constexpr bool Contains(PixelPoint pt) const {
    return pt.x >= left && pt.x < right && pt.y >= top && pt.y < bottom;
  }

  constexpr bool Contains(PixelRect other) const {
    return left <= other.left && top <= other.top &&
      right >= other.right && bottom >= other.bottom;
  }

  constexpr bool OverlapsWith(PixelRect other) const {
    return left < other.right && other.left <= right &&
      top <= other.bottom && other.top <= bottom;
  }

  /**
   * Calculate the top-left point of a rectangle centered inside this
   * one.
   */
  constexpr PixelPoint CenteredTopLeft(PixelSize size) const {
    return PixelPoint((left + right - size.cx) / 2,
                      (top + bottom - size.cy) / 2);
  }

#ifdef USE_WINUSER
  constexpr PixelRect(RECT src)
    :left(src.left), top(src.top), right(src.right), bottom(src.bottom) {}

  operator RECT() const {
    RECT r;
    r.left = left;
    r.top = top;
    r.right = right;
    r.bottom = bottom;
    return r;
  }
#endif
};

#endif
