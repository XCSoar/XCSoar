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

#include "TwoTextRowsRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"

#include <algorithm>

unsigned
TwoTextRowsRenderer::CalculateLayout(const Font &_first_font,
                                     const Font &_second_font)
{
  const unsigned first_font_height = _first_font.GetHeight();
  const unsigned second_font_height = _second_font.GetHeight();
  const unsigned text_padding = Layout::GetTextPadding();
  const unsigned max_height = Layout::GetMaximumControlHeight();
  const unsigned padded_height =
    first_font_height + second_font_height + 3 * text_padding;
  const unsigned row_height = std::max(padded_height, max_height);

  unsigned vertical_padding =
    (row_height - first_font_height - second_font_height) / 3;

  first_font = &_first_font;
  second_font = &_second_font;
  x = text_padding;
  first_y = vertical_padding;
  second_y = first_y + first_font_height + vertical_padding;

  return row_height;
}

void
TwoTextRowsRenderer::DrawFirstRow(Canvas &canvas, const PixelRect &rc,
                                  const TCHAR *text) const
{
  canvas.Select(*first_font);
  canvas.DrawClippedText(rc.left + x, rc.top + first_y, rc, text);
}

void
TwoTextRowsRenderer::DrawSecondRow(Canvas &canvas, const PixelRect &rc,
                                   const TCHAR *text) const
{
  canvas.Select(*second_font);
  canvas.DrawClippedText(rc.left + x, rc.top + second_y, rc, text);
}

int
TwoTextRowsRenderer::DrawRightFirstRow(Canvas &canvas, const PixelRect &rc,
                                       const TCHAR *text) const
{
  canvas.Select(*second_font);
  int text_width = canvas.CalcTextWidth(text);
  int text_x = rc.right - x - text_width;
  if (text_x < rc.left)
    /* text is too large: skip it completely (is there something
       better we can do?) */
    return rc.right;

  canvas.DrawText(text_x, rc.top + first_y, text);
  return text_x - x;
}

int
TwoTextRowsRenderer::DrawRightSecondRow(Canvas &canvas, const PixelRect &rc,
                                        const TCHAR *text) const
{
  canvas.Select(*second_font);
  int text_width = canvas.CalcTextWidth(text);
  int text_x = rc.right - x - text_width;
  if (text_x < rc.left)
    /* text is too large: skip it completely (is there something
       better we can do?) */
    return rc.right;

  canvas.DrawText(text_x, rc.top + second_y, text);
  return text_x - x;
}
