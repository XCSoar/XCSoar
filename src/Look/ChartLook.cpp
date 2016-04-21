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

#include "ChartLook.hpp"
#include "FontDescription.hpp"
#include "Screen/Layout.hpp"

#include <algorithm>

void
ChartLook::Initialise()
{
  pens[STYLE_BLUETHIN].Create(Pen::DASH, 2, Color(0, 50, 255));
  pens[STYLE_REDTHICK].Create(Pen::DASH, 3, Color(200, 50, 50));
  pens[STYLE_DASHGREEN].Create(Pen::DASH, 2, COLOR_GREEN);
  pens[STYLE_MEDIUMBLACK].Create(2, COLOR_BLACK);
  pens[STYLE_GRID].Create(Pen::DASH, 1, Color(0xB0, 0xB0, 0xB0));
  pens[STYLE_GRIDZERO].Create(2, Color(0xB0, 0xB0, 0xB0));

  bar_brush.Create(COLOR_GREEN);
  neg_brush.Create(COLOR_RED);
  blank_brush.Create(Color(0xD0, 0xD0, 0xD0));
  black_brush.Create(COLOR_BLACK);

  int axis_label_size = std::max(8u, Layout::FontScale(6u));
  int axis_value_size = std::max(8u, Layout::FontScale(7u));

  label_font.Load(FontDescription(Layout::FontScale(12)));
  axis_label_font.Load(FontDescription(axis_label_size, true));
  axis_value_font.Load(FontDescription(axis_value_size));

  color_positive = Color(0xa0, 0xd0, 0xf3);
  color_negative = Color(0xf3, 0xd0, 0xa0);
}
