/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_TWO_TEXT_ROWS_RENDERER_HPP
#define XCSOAR_TWO_TEXT_ROWS_RENDERER_HPP

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
  unsigned CalculateLayout(const Font &_first_font, const Font &_second_font);

  const Font &GetFirstFont() const {
    return *first_font;
  }

  const Font &GetSecondFont() const {
    return *second_font;
  }

  int GetX() const {
    return x;
  }

  int GetFirstY() const {
    return first_y;
  }

  int GetSecondY() const {
    return second_y;
  }

  void DrawFirstRow(Canvas &canvas, const PixelRect &rc,
                    const TCHAR *text) const;

  void DrawSecondRow(Canvas &canvas, const PixelRect &rc,
                     const TCHAR *text) const;

  /**
   * Draws a right-aligned column in the first row (but with the
   * second font which is usually smaller) and returns the new "right"
   * coordinate.
   */
  int DrawRightFirstRow(Canvas &canvas, const PixelRect &rc,
                        const TCHAR *text) const;

  /**
   * Draws a right-aligned column in the second row and returns the
   * new "right" coordinate.
   */
  int DrawRightSecondRow(Canvas &canvas, const PixelRect &rc,
                         const TCHAR *text) const;

};

#endif
