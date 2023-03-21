// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "io/FileDescriptor.hxx"

#include <cstdint>

struct gbm_bo;

namespace EGL {

class DrmFrameBuffer {
  const FileDescriptor dri_fd;
  uint32_t id;

public:
  /**
   * Throws on error.
   */
  DrmFrameBuffer(FileDescriptor _dri_fd,
                 uint32_t width, uint32_t height,
                 uint8_t depth, uint8_t bpp,
                 uint32_t pitch,
                 uint32_t bo_handle);

  ~DrmFrameBuffer() noexcept;

  DrmFrameBuffer(const DrmFrameBuffer &) = delete;
  DrmFrameBuffer &operator=(const DrmFrameBuffer &) = delete;

  uint32_t GetId() const noexcept {
    return id;
  }
};

} // namespace EGL
