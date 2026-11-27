// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct UnsignedPoint2D;

namespace SDL {

class Display {
public:
  explicit Display(unsigned antialiasing_samples = 0);
  ~Display() noexcept;

  [[gnu::pure]]
  static UnsignedPoint2D GetDPI() noexcept;

  static void DisableAntiAliasing() noexcept;
};

} // namespace SDL
