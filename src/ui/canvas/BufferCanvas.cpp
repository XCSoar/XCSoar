// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/BufferCanvas.hpp"

#include <algorithm>

void
BufferCanvas::Grow(PixelSize new_size) noexcept
{
  const unsigned old_width = GetWidth();
  const unsigned old_height = GetHeight();

  Resize({std::max(old_width, new_size.width), std::max(old_height, new_size.height)});
}
