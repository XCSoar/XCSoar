// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct UnsignedPoint2D;

namespace SDL {

class Display {
public:
  Display();
  ~Display() noexcept;
  
  [[gnu::pure]]
  UnsignedPoint2D GetDPI() const noexcept;
};

} // namespace SDL
