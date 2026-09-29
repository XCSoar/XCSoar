// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Rect.hpp"

namespace Android {

/**
 * How far the top of the XCSoar view still lies inside Android's
 * system swipe-down band, in pixels.  Zero when that band does not
 * cover the view.
 */
[[nodiscard]]
int
GetTopGestureClearance() noexcept;

/**
 * Where arrange cards and their text may sit.  The top moves down by
 * #GetTopGestureClearance() so a downward drag is not taken by the
 * notification shade.  The caller keeps its own window full screen
 * when the backdrop must still cover that band.
 */
[[nodiscard]]
inline PixelRect
ContentRectBelowTopGesture(PixelRect full) noexcept
{
  const int clearance = GetTopGestureClearance();
  if (clearance > 0 && full.bottom - full.top > clearance)
    full.top += clearance;

  return full;
}

} // namespace Android
