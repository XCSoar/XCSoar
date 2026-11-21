// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Color.hpp"
#include "ui/dim/Rect.hpp"
#include "util/StaticArray.hxx"

class Canvas;
class WindowProjection;
class Font;
class MaskedIcon;
struct OverlayLook;

/** One lower-left label. @c right_icon is the scale's far chevron. */
struct MapLabel {
  const char *text = "";
  const MaskedIcon *icon = nullptr;
  Color fill = COLOR_WHITE;
  const MaskedIcon *right_icon = nullptr;
  unsigned progress_position = 0;
  unsigned progress_range = 0;
};

[[gnu::pure]]
PixelSize
MapLabelSize(const Font &font, const MapLabel &label,
             unsigned max_width = 0) noexcept;

/** shortcut: 8 labels; raise the cap when a ninth condition is added */
using MapLabelList = StaticArray<MapLabel, 8>;

/**
 * Height of the map-scale bar band inside #RenderMapScale /
 * #DrawMapScale.
 */
[[gnu::pure]]
unsigned
GetMapScaleBandHeight(const Font &font) noexcept;

/**
 * Draw one label. @p bottom is its bottom edge. The rectangle lets
 * the next label sit above it or to its right.
 */
PixelRect
DrawMapLabel(Canvas &canvas, const Font &font,
            int left, int bottom, const MapLabel &label,
            unsigned max_width = 0) noexcept;

/**
 * Draw @p list left to right. The gap is two text paddings, the same
 * as between the range and the contour spacing. Returns the right
 * edge of the last label, or @p left when @p list is empty.
 */
int
DrawMapLabelRow(Canvas &canvas, const Font &font,
               int left, int bottom,
               const MapLabelList &list) noexcept;

/**
 * How far above the bottom of the scale rectangle the condition stack
 * must stay: the scale label, a gap, the title label, and a gap.
 */
[[gnu::pure]]
unsigned
GetMapScaleAndTitleClearance(const Font &font) noexcept;

/**
 * Draw the scale, then the contour spacing when @p contour_spacing_m
 * is set, then @p tail when it is set. Returns the right edge of the
 * last label, or @c rc.left when the scale is not drawn.
 */
int
RenderMapScale(Canvas &canvas,
               const WindowProjection& projection,
               const PixelRect &rc,
               const OverlayLook &look,
               unsigned contour_spacing_m = 0,
               const MapLabel *tail = nullptr);
