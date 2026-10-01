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
 * Where cards and their text may sit.  @p top_on_view is the Y of
 * @p rc.top in the XCSoar view.  The top moves down only by the part
 * of the swipe-down band that still covers @p rc, so a dialog already
 * placed in the safe area is not inset a second time.  The caller
 * keeps its own window full screen when the backdrop must still cover
 * that band.
 */
[[nodiscard]]
inline PixelRect
ContentRectBelowTopGesture(PixelRect rc, int top_on_view) noexcept
{
  const int overlap = GetTopGestureClearance() - top_on_view;
  if (overlap > 0 && rc.bottom - rc.top > overlap)
    rc.top += overlap;

  return rc;
}

/**
 * Like the overload above, for a rectangle already in view
 * coordinates.
 */
[[nodiscard]]
inline PixelRect
ContentRectBelowTopGesture(PixelRect full) noexcept
{
  return ContentRectBelowTopGesture(full, full.top);
}

} // namespace Android
