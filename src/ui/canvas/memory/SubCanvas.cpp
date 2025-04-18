// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/SubCanvas.hpp"

#include <algorithm>

static unsigned
ClipMax(unsigned limit, int offset, unsigned size) noexcept
{
  return std::min(unsigned(size),
                  unsigned(std::max(int(limit - offset), 0)));
}

SubCanvas::SubCanvas(Canvas &canvas,
                     PixelPoint _offset, PixelSize _size) noexcept
{
  buffer = canvas.buffer;
  buffer.data = buffer.At(_offset.x, _offset.y);
  buffer.size.width = ClipMax(buffer.size.width, _offset.x, _size.width);
  buffer.size.height = ClipMax(buffer.size.height, _offset.y, _size.height);
}
