// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../RawBitmap.hpp"
#include "Canvas.hpp"

#include <cassert>

RawBitmap::RawBitmap(PixelSize _size) noexcept
  :size(_size),
   buffer(new RawColor[size.width * size.height])
{
}

void
RawBitmap::StretchTo(PixelSize src_size,
                     Canvas &dest_canvas, PixelSize dest_size,
                     bool transparent_white,
                     bool use_source_alpha) const noexcept
{
  ConstImageBuffer<ActivePixelTraits> src{
    ActivePixelTraits::const_pointer(GetBuffer()),
    size.width * sizeof(*GetBuffer()),
    size,
  };

  // Prefer per-pixel alpha over transparent_white when available
  if (use_source_alpha)
    dest_canvas.StretchWithSourceAlpha({0, 0}, dest_size,
                                       src, {0, 0}, src_size);
  else if (transparent_white)
    dest_canvas.StretchTransparentWhite({0, 0}, dest_size,
                                        src, {0, 0}, src_size);
  else
    dest_canvas.Stretch({0, 0}, dest_size,
                        src, {0, 0}, src_size);
}
