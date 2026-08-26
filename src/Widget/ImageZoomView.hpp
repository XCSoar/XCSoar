// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Rect.hpp"
#include "Math/Point2D.hpp"

#include <cmath>

#include <algorithm>

class Bitmap;
class Canvas;

/**
 * Shared fit/zoom/pan math and painting for bitmap viewers.
 *
 * The zoom factor is continuous and relative to the scale at which the
 * whole image fits into the canvas.
 */
namespace ImageZoomView {

/**
 * The zoom factor which fits the whole image into the canvas.
 */
static constexpr double FIT_ZOOM_FACTOR = 1;

static constexpr double MAX_ZOOM_FACTOR = 32;

/**
 * The factor of one zoom step (buttons and keys).
 */
static constexpr double ZOOM_STEP_FACTOR = 2;

/**
 * The distance of one pan step (arrow keys), in logical pixels; pass
 * it through Layout::Scale().
 */
static constexpr int PAN_STEP = 50;

constexpr double
ClampZoomFactor(double zoom_factor) noexcept
{
  return std::clamp(zoom_factor, FIT_ZOOM_FACTOR, MAX_ZOOM_FACTOR);
}

/**
 * Is the whole image visible?
 */
constexpr bool
IsFitZoomFactor(double zoom_factor) noexcept
{
  return zoom_factor <= FIT_ZOOM_FACTOR;
}

/**
 * Describes where PaintZoomedBitmap() has put the bitmap on the
 * canvas.  This allows callers to paint overlays on top of the
 * bitmap at a known bitmap position.
 */
struct Layout {
  /** the area of the canvas covered by the bitmap */
  PixelRect screen_rect;

  /**
   * The top-left of the source rectangle passed to Stretch(), in
   * bitmap pixels.  The fractional pan is already in #screen_rect.
   */
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

/**
 * Determine the position in the bitmap which is displayed at the given
 * canvas position.
 */
[[gnu::pure]]
DoublePoint2D
CanvasToBitmap(PixelPoint p, DoublePoint2D view_pos,
               PixelSize canvas_size, PixelSize bitmap_size,
               double zoom_factor) noexcept;

/**
 * Move the view so that the given bitmap position is displayed at the
 * given canvas position; this is what makes an image follow the
 * fingers during a pinch gesture.
 */
void
MoveViewTo(DoublePoint2D bitmap_pos, PixelPoint anchor,
           DoublePoint2D &view_pos,
           PixelSize canvas_size, PixelSize bitmap_size,
           double zoom_factor) noexcept;

/**
 * Adjust the view position when the zoom factor changes, keeping the
 * bitmap position in the centre of the canvas in place.
 */
void
AdjustImageViewOnZoomChange(double old_zoom_factor, double new_zoom_factor,
                            DoublePoint2D &view_pos,
                            PixelSize canvas_size,
                            PixelSize bitmap_size) noexcept;

/**
 * Paint a bitmap with the given zoom factor and pan offset.
 *
 * While zoomed in, this paints up to one source pixel beyond the
 * canvas on each side; the caller must clip (OpenGL does not clip
 * against siblings).
 *
 * @param view_pos Top-left of the visible region in bitmap pixels
 * @param pending_offset Drag/key nudge in screen pixels (applied once, then cleared)
 * @return where the bitmap was painted; undefined if nothing was painted
 */
Layout
PaintZoomedBitmap(Canvas &canvas, const Bitmap &bitmap, double zoom_factor,
                  DoublePoint2D &view_pos,
                    PixelPoint &pending_offset) noexcept;

} // namespace ImageZoomView
