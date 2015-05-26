/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "TextRowRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"

#include <algorithm>

unsigned
TextRowRenderer::CalculateLayout(const Font &font)
{
  const unsigned font_height = font.GetHeight();
  const unsigned text_padding = Layout::GetTextPadding();
  const unsigned max_height = Layout::GetMaximumControlHeight();
  const unsigned padded_height = font_height + 2 * text_padding;
  const unsigned row_height = std::max(padded_height, max_height);

  left_padding = text_padding;
  top_padding = (row_height - font_height) / 2;

  return row_height;
}

void
TextRowRenderer::DrawTextRow(Canvas &canvas, const PixelRect &rc,
                             const TCHAR *text) const
{
    canvas.DrawClippedText(rc.left + left_padding,
                           rc.top + top_padding,
                           rc,
                           text);
}
