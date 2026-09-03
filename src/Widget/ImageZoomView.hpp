// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Rect.hpp"
#include "ui/dim/Size.hpp"
#include "Math/Point2D.hpp"

#include <cmath>

class Bitmap;
class Canvas;

/**
 * Shared fit/zoom/pan math and painting for bitmap viewers.
 */
namespace ImageZoomView {

static constexpr int max_zoom_level = 5;

/**
 * Describes where PaintZoomedBitmap() has put the bitmap on the
 * canvas.  This allows callers to paint overlays on top of the
 * bitmap at a known bitmap position.
 */
struct Layout {
  /** the area of the canvas covered by the bitmap */
  PixelRect screen_rect;

  /** the top-left of the visible region, in bitmap pixels */
  PixelPoint view_pos;

  /**
   * The factors converting bitmap pixels to canvas pixels.  The two
   * axes are tracked separately because Canvas::Stretch() works on
   * integral rectangles: the effective scale is the ratio of the
   * rectangles it was actually given, and rounding can differ per
   * axis.
   */
  DoublePoint2D scale{0, 0};

  constexpr bool IsDefined() const noexcept {
    return scale.x > 0 && scale.y > 0;
  }

  /**
   * Convert a position inside the bitmap to a canvas position.
   */
  [[gnu::pure]]
  PixelPoint BitmapToScreen(DoublePoint2D p) const noexcept {
    return {
      screen_rect.left + int(std::lround((p.x - view_pos.x) * scale.x)),
      screen_rect.top + int(std::lround((p.y - view_pos.y) * scale.y)),
    };
  }
};

void
AdjustImageViewOnZoomChange(int old_zoom, int new_zoom,
                            PixelPoint &view_pos,
                            PixelSize canvas_size,
                            PixelSize bitmap_size) noexcept;

/**
 * Paint a bitmap with discrete zoom levels and pan offset.
 * @param view_pos Top-left of the visible region in bitmap pixels
 * @param pending_offset Drag/key nudge in screen pixels (applied once, then cleared)
 * @return where the bitmap was painted; undefined if nothing was painted
 */
Layout
PaintZoomedBitmap(Canvas &canvas, const Bitmap &bitmap, int zoom,
                    PixelPoint &view_pos,
                    PixelPoint &pending_offset) noexcept;

} // namespace ImageZoomView
