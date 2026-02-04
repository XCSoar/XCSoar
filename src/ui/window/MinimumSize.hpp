// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Size.hpp"

/**
 * XCSoar's minimum supported window size.
 *
 * The minimum is 320x240 in landscape orientation and 240x320 in portrait.
 * This prevents crashes from invalid rectangles when the window is resized
 * too small (issue #2110).
 */
namespace UI {

static constexpr unsigned MIN_WIDTH = 320;
static constexpr unsigned MIN_HEIGHT = 240;

/**
 * Clamp a size to the minimum supported window size, accounting for
 * both landscape and portrait orientations.
 */
constexpr PixelSize
ClampToMinimumSize(PixelSize s) noexcept
{
  if (s.width < s.height) {
    // Portrait orientation: minimum is 240x320
    if (s.width < MIN_HEIGHT)
      s.width = MIN_HEIGHT;
    if (s.height < MIN_WIDTH)
      s.height = MIN_WIDTH;
  } else {
    // Landscape orientation: minimum is 320x240
    if (s.width < MIN_WIDTH)
      s.width = MIN_WIDTH;
    if (s.height < MIN_HEIGHT)
      s.height = MIN_HEIGHT;
  }

  return s;
}

} // namespace UI
