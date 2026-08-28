// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Point.hpp"
#include "ui/dim/Size.hpp"

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
 */
void
PaintZoomedBitmap(Canvas &canvas, const Bitmap &bitmap, double zoom_factor,
                    DoublePoint2D &view_pos,
                    PixelPoint &pending_offset) noexcept;

} // namespace ImageZoomView
