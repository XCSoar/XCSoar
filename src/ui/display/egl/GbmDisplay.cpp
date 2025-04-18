// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GbmDisplay.hpp"
#include "io/FileDescriptor.hxx"

#include <gbm.h>

#include <stdexcept>

namespace EGL {

GbmDisplay::GbmDisplay(FileDescriptor dri_fd)
  :device(gbm_create_device(dri_fd.Get()))
{
  if (device == nullptr)
    throw std::runtime_error("Could not create GBM device");
}

GbmDisplay::~GbmDisplay() noexcept
{
  gbm_device_destroy(device);
}

} // namespace EGL
