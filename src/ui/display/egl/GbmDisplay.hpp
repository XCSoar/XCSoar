// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct gbm_device;
class FileDescriptor;

namespace EGL {

class GbmDisplay {
  struct gbm_device *const device;

public:
  /**
   * Throws on error.
   */
  explicit GbmDisplay(FileDescriptor dri_fd);

  ~GbmDisplay() noexcept;

  auto *GetGbmDevice() const noexcept {
    return device;
  }
};

} // namespace EGL
