// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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

  explicit constexpr PixelSize(UnsignedPoint2D src) noexcept
    :width(src.x), height(src.y) {}

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

  explicit operator UnsignedPoint2D() const noexcept {
    return { width, height };
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
