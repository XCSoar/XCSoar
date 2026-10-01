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
 * How far above the bottom of the scale rectangle the GPS status must
 * stay: the scale bar, then the title drawn ABOVE it.
 */
[[gnu::pure]]
unsigned
GetMapScaleAndTitleClearance(const Font &font) noexcept;

void
RenderMapScale(Canvas &canvas,
               const WindowProjection& projection,
               const PixelRect &rc,
               const OverlayLook &look,
               unsigned contour_spacing_m = 0);
