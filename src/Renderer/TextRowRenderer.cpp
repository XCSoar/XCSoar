// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TextRowRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"

#include <algorithm>

unsigned
TextRowRenderer::CalculateLayout(const Font &font) noexcept
{
  const unsigned font_height = font.GetHeight();
  const unsigned text_padding = Layout::GetTextPadding();
  const unsigned max_height = Layout::GetMaximumControlHeight();
  const unsigned padded_height = font_height + 2 * text_padding;
  const unsigned row_height = std::max(padded_height, max_height);

  left_padding = text_padding * 2;
  top_padding = (row_height - font_height) / 2;

  return row_height;
}

void
TextRowRenderer::DrawTextRow(Canvas &canvas, const PixelRect &rc,
                             const char *text) const noexcept
{
  canvas.DrawClippedText(rc.GetTopLeft() + PixelSize{left_padding, top_padding},
                         rc, text);
}

int
TextRowRenderer::NextColumn(Canvas &canvas, const PixelRect &rc,
                            const char *text) const noexcept
{
  return std::min<int>(rc.left + int(2 * left_padding + canvas.CalcTextWidth(text)),
                       rc.right);
}

int
TextRowRenderer::DrawColumn(Canvas &canvas, const PixelRect &rc,
                            const char *text) const noexcept
{
  DrawTextRow(canvas, rc, text);
  return NextColumn(canvas, rc, text);
}

int
TextRowRenderer::PreviousRightColumn(Canvas &canvas, const PixelRect &rc,
                                     const char *text) const noexcept
{
  int text_width = canvas.CalcTextWidth(text);
  int x = rc.right - int(left_padding + text_width);
  if (x < rc.left)
    /* text is too large: skip it completely (is there something
       better we can do?) */
    return rc.right;

  return x - left_padding;
}

int
TextRowRenderer::DrawRightColumn(Canvas &canvas, const PixelRect &rc,
                                 const char *text) const noexcept
{
  int text_width = canvas.CalcTextWidth(text);
  int x = rc.right - int(left_padding + text_width);
  if (x < rc.left)
    /* text is too large: skip it completely (is there something
       better we can do?) */
    return rc.right;

  canvas.DrawText({x, rc.top + (int)top_padding}, text);
  return x - left_padding;
}
