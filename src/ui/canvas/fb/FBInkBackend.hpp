// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Size.hpp"

#include <cstddef>

class FBInkBackend {
public:
  struct FrameBuffer {
    unsigned char *data;
    std::size_t size;
    unsigned pitch;
    unsigned bytes_per_pixel;
  };

private:
  struct Config;
  Config *config;
  int fd;

public:
  FBInkBackend();
  ~FBInkBackend() noexcept;

  FBInkBackend(const FBInkBackend &) = delete;
  FBInkBackend &operator=(const FBInkBackend &) = delete;

  PixelSize GetSize() const noexcept;

  FrameBuffer Prepare();
  void Refresh(PixelSize size);
};
