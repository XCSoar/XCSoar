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

#include "ui/dim/Size.hpp"
#include "io/UniqueFileDescriptor.hxx"

#include <xf86drm.h>
#include <xf86drmMode.h>

namespace EGL {

class DrmDisplay {
  UniqueFileDescriptor dri_fd;

  drmModeModeInfo mode;

  uint32_t connector_id;
  uint32_t crtc_id;

  PixelSize size_mm;

public:
  /**
   * Throws on error.
   */
  DrmDisplay();

  ~DrmDisplay() noexcept;

  FileDescriptor GetDriFD() const noexcept {
    return dri_fd;
  }

  /**
   * Acquire the DRM master lease.
   *
   * Throws on error.
   */
  void SetMaster();

  /**
   * Drop the DRM master lease.  Allows the next process to open the
   * DRI device to become master.
   */
  void DropMaster() noexcept;

  const auto &GetMode() const noexcept {
    return mode;
  }

  [[gnu::pure]]
  drmModeCrtcPtr ModeGetCrtc() const noexcept {
    return drmModeGetCrtc(dri_fd.Get(), crtc_id);
  }

  int ModeSetCrtc(uint32_t crtcId, uint32_t bufferId,
                  uint32_t x, uint32_t y,
                  drmModeModeInfoPtr mode) noexcept {
    return drmModeSetCrtc(dri_fd.Get(), crtcId, bufferId,
                          x, y,
                          &connector_id, 1,
                          mode);
  }

  int ModeSetCrtc(uint32_t bufferId, uint32_t x, uint32_t y) noexcept {
    return ModeSetCrtc(crtc_id, bufferId, x, y, &mode);
  }

  int ModePageFlip(uint32_t fb_id, uint32_t flags, void *user_data) noexcept {
    return drmModePageFlip(dri_fd.Get(), crtc_id, fb_id, flags, user_data);
  }

  PixelSize GetSize() const noexcept {
    return {mode.hdisplay, mode.vdisplay};
  }

  PixelSize GetSizeMM() const noexcept {
    return size_mm;
  }
};

} // namespace EGL
