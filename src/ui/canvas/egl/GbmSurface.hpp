/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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
