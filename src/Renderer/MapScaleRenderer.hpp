// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Canvas;
class WindowProjection;
class Font;
struct OverlayLook;
struct PixelRect;

/**
 * Height of the map-scale bar band inside #RenderMapScale /
 * #DrawMapScale.
 */
[[gnu::pure]]
unsigned
GetMapScaleBandHeight(const Font &font) noexcept;

/**
 * How far above the bottom of the scale rectangle the condition stack
 * must stay: the scale pill, a gap, the title pill, and a gap.
 */
[[gnu::pure]]
unsigned
GetMapScaleAndTitleClearance(const Font &font) noexcept;

/**
 * Draw the scale, and the contour spacing when @p contour_spacing_m
 * is set.  Returns the right edge of the last label, or @c rc.left
 * when nothing was drawn.
 */
int
RenderMapScale(Canvas &canvas,
               const WindowProjection& projection,
               const PixelRect &rc,
               const OverlayLook &look,
               unsigned contour_spacing_m = 0);
