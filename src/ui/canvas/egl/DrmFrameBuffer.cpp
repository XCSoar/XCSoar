// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
