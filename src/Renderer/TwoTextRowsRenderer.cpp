// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TwoTextRowsRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"

#include <algorithm>

unsigned
TwoTextRowsRenderer::CalculateLayout(const Font &_first_font,
                                     const Font &_second_font) noexcept
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
                                  const char *text) const noexcept
{
  canvas.Select(*first_font);
  canvas.DrawClippedText({rc.left + x, rc.top + first_y}, rc, text);
}

void
TwoTextRowsRenderer::DrawSecondRow(Canvas &canvas, const PixelRect &rc,
                                   const char *text) const noexcept
{
  canvas.Select(*second_font);
  canvas.DrawClippedText({rc.left + x, rc.top + second_y}, rc, text);
}

int
TwoTextRowsRenderer::DrawRightFirstRow(Canvas &canvas, const PixelRect &rc,
                                       const char *text) const noexcept
{
  canvas.Select(*second_font);
  int text_width = canvas.CalcTextWidth(text);
  int text_x = rc.right - x - text_width;
  if (text_x < rc.left)
    /* text is too large: skip it completely (is there something
       better we can do?) */
    return rc.right;

  canvas.DrawText({text_x, rc.top + first_y}, text);
  return text_x - x;
}

int
TwoTextRowsRenderer::DrawRightSecondRow(Canvas &canvas, const PixelRect &rc,
                                        const char *text) const noexcept
{
  canvas.Select(*second_font);
  int text_width = canvas.CalcTextWidth(text);
  int text_x = rc.right - x - text_width;
  if (text_x < rc.left)
    /* text is too large: skip it completely (is there something
       better we can do?) */
    return rc.right;

  canvas.DrawText({text_x, rc.top + second_y}, text);
  return text_x - x;
}
