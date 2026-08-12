// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FBInkBackend.hpp"

#include <fbink.h>

#include <stdexcept>

struct FBInkBackend::Config : FBInkConfig {};

static FBInkState
GetState(const FBInkConfig &config) noexcept
{
  FBInkState state{};
  fbink_get_state(&config, &state);
  return state;
}

FBInkBackend::FBInkBackend()
  :config(new Config{}), fd(fbink_open())
{
  if (fd < 0) {
    delete config;
    throw std::runtime_error("Failed to open the framebuffer with FBInk");
  }

  config->is_quiet = true;
  if (fbink_init(fd, config) < 0) {
    fbink_close(fd);
    delete config;
    throw std::runtime_error("Failed to initialise FBInk");
  }
}

FBInkBackend::~FBInkBackend() noexcept
{
  fbink_close(fd);
  delete config;
}

PixelSize
FBInkBackend::GetSize() const noexcept
{
  const FBInkState state = GetState(*config);
  return {state.screen_width, state.screen_height};
}

FBInkBackend::FrameBuffer
FBInkBackend::Prepare()
{
  if (fbink_reinit(fd, config) < 0)
    throw std::runtime_error("Failed to reinitialise FBInk");

  const FBInkState state = GetState(*config);
  if (state.screen_width == 0 || state.screen_height == 0 ||
      state.scanline_stride == 0 ||
      (state.bpp != 8 && state.bpp != 16 && state.bpp != 32))
    throw std::runtime_error("FBInk reported an unsupported framebuffer");

  std::size_t size = 0;
  unsigned char *data = fbink_get_fb_pointer(fd, &size);
  if (data == nullptr ||
      size < std::size_t{state.scanline_stride} * state.screen_height)
    throw std::runtime_error("FBInk did not provide a complete framebuffer");

  return {data, size, state.scanline_stride, state.bpp / 8};
}

void
FBInkBackend::Refresh(PixelSize size)
{
  if (fbink_refresh(fd, 0, 0, size.width, size.height, config) < 0)
    throw std::runtime_error("FBInk screen refresh failed");
}
