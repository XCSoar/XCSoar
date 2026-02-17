// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/custom/UncompressedImage.hpp"
#include "Screen/Debug.hpp"
#include "UncompressedImage.hpp"

#include <cassert>
#include <utility> // for std::exchange()

Bitmap::Bitmap(Bitmap &&src) noexcept
  :buffer(std::exchange(src.buffer, WritableImageBuffer<BitmapPixelTraits>::Empty())),
   has_colors(src.has_colors)
{
}

Bitmap &Bitmap::operator=(Bitmap &&src) noexcept
{
  using std::swap;
  swap(buffer, src.buffer);
  swap(has_colors, src.has_colors);
  return *this;
}

bool
Bitmap::Load(UncompressedImage &&uncompressed, Type)
{
  assert(IsScreenInitialized());
  assert(uncompressed.IsDefined());

  Reset();

  has_colors = uncompressed.HasNonGrayscalePixels();
  ImportSurface(buffer, uncompressed);
  return true;
}

void
Bitmap::Reset() noexcept
{
  assert(!IsDefined() || IsScreenInitialized());

  buffer.Free();
}
