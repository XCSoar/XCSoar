// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UncompressedImage.hpp"

#include <cstdint>

bool
UncompressedImage::HasNonGrayscalePixels() const noexcept
{
  if (format == Format::GRAY || format == Format::INVALID)
    return false;

  const auto *p = static_cast<const uint8_t *>(GetData());
  const unsigned bpp = (format == Format::RGBA) ? 4 : 3;

  for (unsigned y = 0; y < height; y++) {
    const auto *row = p + y * pitch;
    for (unsigned x = 0; x < width; x++) {
      const unsigned off = x * bpp;
      if (row[off] != row[off + 1] || row[off] != row[off + 2])
        return true;
    }
  }

  return false;
}
