/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#pragma once

#include "Point.hpp"

struct PixelSize {
  unsigned width, height;

  PixelSize() = default;

  explicit constexpr PixelSize(unsigned _size) noexcept
    :width(_size), height(_size) {}

  constexpr PixelSize(int _width, int _height) noexcept
    :width(_width), height(_height) {}

  constexpr PixelSize(unsigned _width, unsigned _height) noexcept
    :width(_width), height(_height) {}

  constexpr PixelSize(long _width, long _height) noexcept
    :width(_width), height(_height) {}

  constexpr bool operator==(const PixelSize &other) const noexcept {
    return width == other.width && height == other.height;
  }

  constexpr bool operator!=(const PixelSize &other) const noexcept {
    return !(*this == other);
  }

  constexpr PixelSize operator+(PixelSize other) const noexcept {
    return {width + other.width, height + other.height};
  }

  constexpr PixelSize operator*(unsigned v) const noexcept {
    return {width * v, height * v};
  }

  constexpr PixelSize operator/(unsigned v) const noexcept {
    return {width / v, height / v};
  }

  constexpr PixelSize operator*(float v) const noexcept {
    return {unsigned(width * v), unsigned(height * v)};
  }

  constexpr PixelSize operator/(float v) const noexcept {
    return {unsigned(width / v), unsigned(height / v)};
  }
};

constexpr PixelPoint
operator+(PixelPoint p, PixelSize size) noexcept
{
  return p.At(size.width, size.height);
}

constexpr PixelPoint
operator-(PixelPoint p, PixelSize size) noexcept
{
  return p.At(-int(size.width), -int(size.height));
}
