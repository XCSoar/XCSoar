// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../RawBitmap.hpp"
#include "Canvas.hpp"

#include <algorithm>
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
                     bool use_source_alpha,
                     float alpha) const noexcept
{
  ConstImageBuffer<ActivePixelTraits> src{
    ActivePixelTraits::const_pointer(GetBuffer()),
    size.width * sizeof(*GetBuffer()),
    size,
  };

  uint8_t alpha_u8 =
    static_cast<uint8_t>(std::clamp(alpha, 0.0f, 1.0f) * 255 + 0.5f);

  if (use_source_alpha) {
    if (alpha_u8 < 255)
      /* combine per-pixel source alpha with the global opacity, matching
         the OpenGL path */
      dest_canvas.StretchWithSourceAlpha({0, 0}, dest_size,
                                         src, {0, 0}, src_size,
                                         alpha_u8);
    else
      dest_canvas.StretchWithSourceAlpha({0, 0}, dest_size,
                                         src, {0, 0}, src_size);
  } else if (alpha_u8 < 255) {
    if (transparent_white)
      dest_canvas.AlphaBlendNotWhite({0, 0}, dest_size,
                                     src, {0, 0}, src_size,
                                     alpha_u8);
    else
      dest_canvas.AlphaBlend({0, 0}, dest_size,
                             src, {0, 0}, src_size,
                             alpha_u8);
  } else if (transparent_white)
    dest_canvas.StretchTransparentWhite({0, 0}, dest_size,
                                        src, {0, 0}, src_size);
  else
    dest_canvas.Stretch({0, 0}, dest_size,
                        src, {0, 0}, src_size);
}
