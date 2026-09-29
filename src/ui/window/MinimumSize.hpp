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
 * Minimum the window manager should enforce for this orientation.
 * Landscape is 320x240, portrait is 240x320. A square window uses
 * the landscape minimum.
 */
constexpr PixelSize
MinimumWindowSize(PixelSize s) noexcept
{
  if (s.width < s.height)
    return {MIN_HEIGHT, MIN_WIDTH};

  return {MIN_WIDTH, MIN_HEIGHT};
}

/**
 * Clamp a size to the minimum supported window size, accounting for
 * both landscape and portrait orientations.
 */
constexpr PixelSize
ClampToMinimumSize(PixelSize s) noexcept
{
  const PixelSize min = MinimumWindowSize(s);

  if (s.width < min.width)
    s.width = min.width;
  if (s.height < min.height)
    s.height = min.height;

  return s;
}

} // namespace UI
