// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct _XDisplay;
struct PixelSize;

namespace X11 {

class Display {
  _XDisplay *const display;

public:
  /**
   * Throws on error.
   */
  Display();

  ~Display() noexcept;

  auto GetXDisplay() const noexcept {
    return display;
  }

  [[gnu::pure]]
  PixelSize GetSize() const noexcept;

  /**
   * Returns the display size in mm.
   */
  [[gnu::pure]]
  PixelSize GetSizeMM() const noexcept;
};

} // namespace X11
