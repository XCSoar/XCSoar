// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Size.hpp"

#include <cstdint>

struct wl_display;
struct wl_output;
struct wl_registry;

namespace Wayland {

class Display {
  friend void OutputGeometry(void *, struct wl_output *, int32_t, int32_t,
                             int32_t, int32_t, int32_t, const char *,
                             const char *, int32_t) noexcept;
  friend void OutputMode(void *, struct wl_output *, uint32_t, int32_t,
                         int32_t, int32_t) noexcept;
  friend void OutputDone(void *, struct wl_output *) noexcept;
  friend void OutputScale(void *, struct wl_output *, int32_t) noexcept;
  friend void RegistryGlobal(void *, struct wl_registry *, uint32_t,
                             const char *, uint32_t);

  struct wl_display *const display;

  mutable struct wl_output *output = nullptr;
  mutable PixelSize size{0, 0};
  mutable PixelSize size_mm{0, 0};
  mutable bool output_initialized = false;

  void InitOutput() const noexcept;

public:
  /**
   * Throws on error.
   */
  Display();

  ~Display() noexcept;

  auto GetWaylandDisplay() noexcept {
    return display;
  }

  /**
   * Returns the display size in pixels.
   */
  PixelSize GetSize() const noexcept;

  /**
   * Returns the display size in mm.
   */
  PixelSize GetSizeMM() const noexcept;
};

} // namespace Wayland
