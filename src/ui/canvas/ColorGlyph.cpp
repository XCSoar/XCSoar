// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ColorGlyph.hpp"

bool
RenderColorGlyph([[maybe_unused]] const char *text,
                 [[maybe_unused]] unsigned size,
                 [[maybe_unused]] Bitmap &bitmap) noexcept
{
  /* the text stack of this platform has no color fonts */
  return false;
}
