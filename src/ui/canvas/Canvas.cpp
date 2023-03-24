// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Canvas.hpp"
#include "Bitmap.hpp"
#include "Screen/Layout.hpp"

void
Canvas::ScaleCopy(PixelPoint dest_position,
                  const Bitmap &src,
                  PixelPoint src_position, PixelSize src_size) noexcept
{
  if (Layout::ScaleEnabled())
    Stretch(dest_position, Layout::Scale(src_size),
            src, src_position, src_size);
  else
    Copy(dest_position, src_size,
         src, src_position);
}
