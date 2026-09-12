// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

struct PixelSize;
struct wl_display;
struct wl_output;
struct wl_registry;
struct zxdg_output_manager_v1;
struct zxdg_output_v1;

namespace Wayland {

/**
 * First wl_output and its xdg-output.
 *
 * GetSize() is compositor-local pixels (xdg-output logical size).
 * GetHardwareSize() is the wl_output mode.  Layout/DPI use the
 * hardware size so fonts are rasterised at the panel density;
 * fractional scale only sizes the EGL buffer and the viewport.
 */
class Display {
  struct wl_display *const display;

  struct wl_output *output = nullptr;
  struct zxdg_output_manager_v1 *xdg_output_manager = nullptr;
  struct zxdg_output_v1 *xdg_output = nullptr;

  unsigned width = 0, height = 0;
  unsigned mm_width = 0, mm_height = 0;
  unsigned logical_width = 0, logical_height = 0;
  unsigned scale = 1;
  int32_t transform = 0;

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
   * Compositor-local output size, or 0x0 if unknown.
   */
  [[gnu::pure]]
  PixelSize GetSize() const noexcept;

  /**
   * wl_output hardware mode, or 0x0 if unknown.
   */
  [[gnu::pure]]
  PixelSize GetHardwareSize() const noexcept;

  /**
   * Physical size in millimetres, or 0x0 if unknown.
   */
  [[gnu::pure]]
  PixelSize GetSizeMM() const noexcept;

  /**
   * Output scale in 120ths (120 = 100%, 240 = 200%).  Derived from
   * xdg-output logical size vs hardware mode, else wl_output integer
   * scale.
   */
  [[gnu::pure]]
  unsigned GetScale120() const noexcept;

  void RegistryHandler(struct wl_registry *registry, uint32_t id,
                       const char *interface, uint32_t version) noexcept;
  void OutputGeometry(int32_t physical_width, int32_t physical_height,
                      int32_t transform) noexcept;
  void OutputMode(uint32_t flags, int32_t width, int32_t height) noexcept;
  void OutputScale(int32_t factor) noexcept;
  void OutputLogicalSize(int32_t width, int32_t height) noexcept;

private:
  void BindXdgOutput() noexcept;
};

} // namespace Wayland
