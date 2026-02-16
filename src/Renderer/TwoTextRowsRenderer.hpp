// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

struct PixelRect;
class Font;
class Canvas;

/**
 * A helper for drawing two text rows into a rectangular area.
 */
class TwoTextRowsRenderer {
  const Font *first_font, *second_font;

  int x, first_y, second_y;

public:
  /**
   * @return the row height (including top and bottom padding)
   */
  unsigned CalculateLayout(const Font &_first_font,
                           const Font &_second_font) noexcept;

  const Font &GetFirstFont() const noexcept {
    return *first_font;
  }

  const Font &GetSecondFont() const noexcept {
    return *second_font;
  }

  int GetX() const noexcept {
    return x;
  }

  int GetFirstY() const noexcept {
    return first_y;
  }

  int GetSecondY() const noexcept {
    return second_y;
  }

  void DrawFirstRow(Canvas &canvas, const PixelRect &rc,
                    const char *text) const noexcept;

  void DrawSecondRow(Canvas &canvas, const PixelRect &rc,
                     const char *text) const noexcept;

  /**
   * Draws a right-aligned column in the first row (but with the
   * second font which is usually smaller) and returns the new "right"
   * coordinate.
   */
  int DrawRightFirstRow(Canvas &canvas, const PixelRect &rc,
                        const char *text) const noexcept;

  /**
   * Draws a right-aligned column in the second row and returns the
   * new "right" coordinate.
   */
  int DrawRightSecondRow(Canvas &canvas, const PixelRect &rc,
                         const char *text) const noexcept;
};
