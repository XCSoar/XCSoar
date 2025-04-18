// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

struct gbm_device;
struct gbm_surface;

namespace EGL {

class GbmSurface {
  struct gbm_surface *const surface;

public:
  /**
   * Throws on error.
   */
  GbmSurface(struct gbm_device *gbm,
             uint32_t width, uint32_t height,
             uint32_t format, uint32_t flags);
  ~GbmSurface() noexcept;

  GbmSurface(const GbmSurface &) = delete;
  GbmSurface &operator=(const GbmSurface &) = delete;

  operator struct gbm_surface *() const noexcept {
    return surface;
  }
};

} // namespace EGL
