// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"

#include <cstdint>

struct PixelSize;
struct wl_display;
struct wl_output;
struct wl_registry;

namespace Wayland {

/**
 * First wl_output.  GetSize() is compositor-local pixels (hardware
 * mode divided by the integer wl_output scale).
 */
class Display {
  struct wl_display *const display;

  struct wl_output *output = nullptr;

  unsigned width = 0, height = 0;
  unsigned mm_width = 0, mm_height = 0;
  unsigned refresh_mhz = 0;
  unsigned scale = 1;

  StaticString<64> make;
  StaticString<64> model;

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
   * Compositor-local size for Layout/DPI, or 0x0 if unknown.
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

  [[gnu::pure]]
  unsigned GetScale() const noexcept {
    return scale > 0 ? scale : 1u;
  }

  [[gnu::pure]]
  unsigned GetRefreshMilliHz() const noexcept {
    return refresh_mhz;
  }

  [[gnu::pure]]
  const char *GetMake() const noexcept {
    return make.c_str();
  }

  [[gnu::pure]]
  const char *GetModel() const noexcept {
    return model.c_str();
  }

  void RegistryHandler(struct wl_registry *registry, uint32_t id,
                       const char *interface, uint32_t version) noexcept;
  void OutputGeometry(int32_t physical_width, int32_t physical_height,
                      const char *make_name,
                      const char *model_name) noexcept;
  void OutputMode(uint32_t flags, int32_t width, int32_t height,
                  int32_t refresh) noexcept;
  void OutputScale(int32_t factor) noexcept;
};

} // namespace Wayland
