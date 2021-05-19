/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_UI_RECT_HPP
#define XCSOAR_UI_RECT_HPP

#include "Point.hpp"
#include "Size.hpp"

#include <utility>

#ifdef USE_WINUSER
#include <windef.h>
#endif

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

  constexpr PixelRect(int _left, int _top, int _right, int _bottom) noexcept
    :left(_left), top(_top), right(_right), bottom(_bottom) {}

  constexpr PixelRect(PixelPoint origin, PixelSize size) noexcept
    :left(origin.x), top(origin.y),
     right(origin.x + size.width), bottom(origin.y + size.height) {}

  explicit constexpr PixelRect(PixelSize size) noexcept
    :left(0), top(0), right(size.width), bottom(size.height) {}

  /**
   * Construct an empty rectangle at the given position.
   */
  explicit constexpr PixelRect(PixelPoint origin) noexcept
    :left(origin.x), top(origin.y), right(origin.x), bottom(origin.y) {}

  /**
   * Construct a #PixelRect that is centered at the given #PixelPoint
   * with the given (total) size.
   */
  static constexpr PixelRect Centered(PixelPoint center, PixelSize size) noexcept {
    const PixelPoint top_left = center - size / 2u;
    return {top_left, size};
  }

  bool IsEmpty() noexcept {
    return left >= right || top >= bottom;
  }

  void SetEmpty() noexcept {
    left = top = right = bottom = 0;
  }

  void Offset(int dx, int dy) noexcept {
    left += dx;
    top += dy;
    right += dx;
    bottom += dy;
  }

  void Grow(int dx, int dy) noexcept {
    left -= dx;
    top -= dy;
    right += dx;
    bottom += dy;
  }

  void Grow(int d) noexcept {
    Grow(d, d);
  }

  constexpr PixelPoint GetOrigin() const noexcept {
    return { left, top };
  }

  constexpr PixelSize GetSize() const noexcept {
    return { right - left, bottom - top };
  }

  constexpr PixelPoint GetCenter() const noexcept {
    return { (left + right) / 2, (top + bottom) / 2 };
  }

  constexpr PixelPoint GetTopLeft() const noexcept {
    return { left, top };
  }

  constexpr PixelPoint GetTopMiddle() const noexcept {
    return { (left + right) / 2, top };
  }

  constexpr PixelPoint GetTopRight() const noexcept {
    return { right, top };
  }

  constexpr PixelPoint GetMiddleLeft() const noexcept {
    return { left, (top + bottom) / 2 };
  }

  constexpr PixelPoint GetMiddleRight() const noexcept {
    return { right, (top + bottom) / 2 };
  }

  constexpr PixelPoint GetBottomLeft() const noexcept {
    return { left, bottom };
  }

  constexpr PixelPoint GetBottomMiddle() const noexcept {
    return { (left + right) / 2, bottom };
  }

  constexpr PixelPoint GetBottomRight() const noexcept {
    return { right, bottom };
  }

  constexpr unsigned GetWidth() const noexcept {
    return right - left;
  }

  constexpr unsigned GetHeight() const noexcept {
    return bottom - top;
  }

  constexpr bool Contains(PixelPoint pt) const noexcept {
    return pt.x >= left && pt.x < right && pt.y >= top && pt.y < bottom;
  }

  constexpr bool Contains(PixelRect other) const noexcept {
    return left <= other.left && top <= other.top &&
      right >= other.right && bottom >= other.bottom;
  }

  constexpr bool OverlapsWith(PixelRect other) const noexcept {
    return left < other.right && other.left <= right &&
      top <= other.bottom && other.top <= bottom;
  }

  /**
   * Return a new #PixelRect grown by this number of pixels.
   */
  constexpr PixelRect WithMargin(PixelSize margin) const noexcept {
    return {
      left - (int)margin.width,
      top - (int)margin.height,
      right + (int)margin.width,
      bottom + (int)margin.height,
    };
  }

  constexpr PixelRect WithMargin(int margin) const noexcept {
    return WithMargin({margin, margin});
  }

  /**
   * Return a new #PixelRect shrunk by this number of pixels.
   */
  constexpr PixelRect WithPadding(PixelSize padding) const noexcept {
    return {
      left + (int)padding.width,
      top + (int)padding.height,
      right - (int)padding.width,
      bottom - (int)padding.height,
    };
  }

  constexpr PixelRect WithPadding(int padding) const noexcept {
    return WithPadding({padding, padding});
  }

  /**
   * Calculate the top-left point of a rectangle centered inside this
   * one.
   */
  constexpr PixelPoint CenteredTopLeft(PixelSize size) const noexcept {
    return PixelPoint((left + right - (int)size.width) / 2,
                      (top + bottom - (int)size.height) / 2);
  }

  constexpr std::pair<PixelRect, PixelRect> VerticalSplit(int x) const noexcept {
    PixelRect a = *this, b = *this;
    a.right = b.left = x;
    return {a, b};
  }

  constexpr std::pair<PixelRect, PixelRect> HorizontalSplit(int y) const noexcept {
    PixelRect a = *this, b = *this;
    a.bottom = b.top = y;
    return {a, b};
  }

#ifdef USE_WINUSER
  constexpr PixelRect(RECT src) noexcept
    :left(src.left), top(src.top), right(src.right), bottom(src.bottom) {}

  constexpr operator RECT() const noexcept {
    RECT r{};
    r.left = left;
    r.top = top;
    r.right = right;
    r.bottom = bottom;
    return r;
  }
#endif
};

#endif
