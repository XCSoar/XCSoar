/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Screen/UnitSymbol.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"

#include <assert.h>

PixelSize
UnitSymbol::GetScreenSize() const
{
  return { Layout::Scale(size.cx), Layout::Scale(size.cy) };
}

void 
UnitSymbol::Draw(Canvas &canvas, PixelScalar x, PixelScalar y, Kind kind) const
{
  Color text_color = COLOR_BLACK, bg_color = COLOR_WHITE;
  if (kind & INVERSE)
    std::swap(text_color, bg_color);
  if (kind & GRAY)
    text_color = COLOR_GRAY;

  const PixelSize size = GetSize();
  const PixelSize screen_size = GetScreenSize();
  canvas.StretchMono(x, y, screen_size.cx, screen_size.cy,
                     bitmap, 0, 0, size.cx, size.cy,
                     text_color, bg_color);
}
