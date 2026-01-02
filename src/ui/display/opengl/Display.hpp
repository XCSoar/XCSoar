// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

namespace OpenGL {

class Display {
public:
  explicit Display(unsigned antialiasing_samples = 0);
  ~Display() noexcept;
};

} // namespace OpenGL
