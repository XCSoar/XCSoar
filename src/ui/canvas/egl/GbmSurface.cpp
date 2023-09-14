// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GbmSurface.hpp"

#include <stdexcept>

#include <gbm.h>

namespace EGL {

GbmSurface::GbmSurface(struct gbm_device *gbm,
                       uint32_t width, uint32_t height,
                       uint32_t format, uint32_t flags)
  :surface(gbm_surface_create(gbm, width, height, format, flags))
{
  if (surface == nullptr)
    throw std::runtime_error("Could not create GBM surface");
}

GbmSurface::~GbmSurface() noexcept
{
  gbm_surface_destroy(surface);
}

} // namespace EGL
