// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

struct PixelRect;
class Font;
class Canvas;

/**
 * A helper for drawing a text row into a rectangular area.
 */
class TextRowRenderer {
  unsigned left_padding, top_padding;

public:
  /**
   * @return the row height (including top and bottom padding)
   */
  unsigned CalculateLayout(const Font &font) noexcept;

  void DrawTextRow(Canvas &canvas, const PixelRect &rc,
                   const char *text) const noexcept;

  /**
   * Returns the minimum X coordinate of the column after the given
   * text.
   */
  [[gnu::pure]]
  int NextColumn(Canvas &canvas, const PixelRect &rc,
                 const char *text) const noexcept;

  /**
   * Combine DrawTextRow() and NextColumn().
   */
  int DrawColumn(Canvas &canvas, const PixelRect &rc,
                 const char *text) const noexcept;

  /**
   * Returns the maximum X coordinate of the column before the given
   * text.
   */
  [[gnu::pure]]
  int PreviousRightColumn(Canvas &canvas, const PixelRect &rc,
                          const char *text) const noexcept;

  /**
   * Draws a right-aligned column and returns the new "right"
   * coordinate.
   */
  int DrawRightColumn(Canvas &canvas, const PixelRect &rc,
                      const char *text) const noexcept;
};
