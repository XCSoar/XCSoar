// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Size.hpp"

class Bitmap;
class Canvas;
struct PixelPoint;

/**
 * Shared fit/zoom/pan math and painting for bitmap viewers.
 */
namespace ImageZoomView {

static constexpr int max_zoom_level = 5;

void
AdjustImageViewOnZoomChange(int old_zoom, int new_zoom,
                            PixelPoint &view_pos,
                            PixelSize canvas_size,
                            PixelSize bitmap_size) noexcept;

/**
 * Paint a bitmap with discrete zoom levels and pan offset.
 * @param view_pos Top-left of the visible region in bitmap pixels
 * @param pending_offset Drag/key nudge in screen pixels (applied once, then cleared)
 */
void
PaintZoomedBitmap(Canvas &canvas, const Bitmap &bitmap, int zoom,
                    PixelPoint &view_pos,
                    PixelPoint &pending_offset) noexcept;

} // namespace ImageZoomView
