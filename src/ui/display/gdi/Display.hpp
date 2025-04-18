// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct UnsignedPoint2D;
struct PixelSize;

namespace GDI {

class Display {
public:
  [[gnu::pure]]
  PixelSize GetSize() const noexcept;

  [[gnu::pure]]
  UnsignedPoint2D GetDPI() const noexcept;
};

} // namespace GDI
