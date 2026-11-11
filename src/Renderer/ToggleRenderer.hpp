// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Color.hpp"
#include "ui/dim/Rect.hpp"

struct DialogLook;
class Canvas;

/**
 * The height of the switch which shows a boolean, fitted into a row
 * of @p row_height.  It stays clear of the text padding, and it is
 * even so the two half circles meet the track without a step.
 */
[[nodiscard]] [[gnu::pure]]
int
ToggleHeight(int row_height) noexcept;

/** The width of that switch.  The track is much wider than tall. */
[[nodiscard]] [[gnu::pure]]
int
ToggleWidth(int height) noexcept;

/**
 * Draw the switch.  @p background_color and @p text_color are the
 * colors of the row it sits on; a display which knows only those two
 * colors uses them, and a color display uses the switch colors.
 */
void
DrawToggle(Canvas &canvas, const PixelRect &rc, bool checked,
           const DialogLook &look, Color background_color,
           Color text_color) noexcept;
