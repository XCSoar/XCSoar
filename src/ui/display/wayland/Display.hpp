// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct wl_display;

namespace Wayland {

class Display {
  struct wl_display *const display;

public:
  /**
   * Throws on error.
   */
  Display();

  ~Display() noexcept;

  auto GetWaylandDisplay() noexcept {
    return display;
  }
};

} // namespace Wayland
