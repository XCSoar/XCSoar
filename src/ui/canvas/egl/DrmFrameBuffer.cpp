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

#include "DrmFrameBuffer.hpp"

#include "xf86drmMode.h"

#include <stdexcept>

namespace EGL {

DrmFrameBuffer::DrmFrameBuffer(FileDescriptor _dri_fd,
                               uint32_t width, uint32_t height,
                               uint8_t depth, uint8_t bpp,
                               uint32_t pitch,
                               uint32_t bo_handle)
  :dri_fd(_dri_fd)
{
  if (drmModeAddFB(dri_fd.Get(),
                   width, height, depth, bpp, pitch, bo_handle,
                   &id) != 0)
    throw std::runtime_error{"drmModeAddFB() failed"};
}

DrmFrameBuffer::~DrmFrameBuffer() noexcept
{
  drmModeRmFB(dri_fd.Get(), id);
}

} // namespace EGL
