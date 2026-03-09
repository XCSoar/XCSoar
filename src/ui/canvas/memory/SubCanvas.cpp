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

  int offset_x = _offset.x;
  int offset_y = _offset.y;
  unsigned size_width = _size.width;
  unsigned size_height = _size.height;

  if (offset_x < 0) {
    const unsigned clipped = std::min(size_width, unsigned(-offset_x));
    size_width -= clipped;
    offset_x = 0;
  }

  if (offset_y < 0) {
    const unsigned clipped = std::min(size_height, unsigned(-offset_y));
    size_height -= clipped;
    offset_y = 0;
  }

  buffer.size.width = ClipMax(buffer.size.width, offset_x, size_width);
  buffer.size.height = ClipMax(buffer.size.height, offset_y, size_height);

  if (buffer.size.width == 0 || buffer.size.height == 0) {
    /* Do not leave buffer.data pointing at the parent: IsDefined() must be
       false for a zero-sized subcanvas. */
    buffer = WritableImageBuffer<ActivePixelTraits>::Empty();
    return;
  }

  buffer.data = buffer.At(offset_x, offset_y);
}
