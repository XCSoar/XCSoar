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
  const auto width_thin = Layout::ScalePenWidth(1);
  const auto width_normal = Layout::ScalePenWidth(2);
  const auto width_thick = Layout::ScalePenWidth(3);

  pens[STYLE_BLUETHINDASH].Create(Pen::DASH2, width_thin, Color(0, 50, 255));
  pens[STYLE_BLUEDASH].Create(Pen::DASH2, width_normal, Color(0, 50, 255));
  pens[STYLE_BLUE].Create(width_normal, Color(0, 50, 255));

  pens[STYLE_REDTHICKDASH].Create(Pen::DASH3, width_thick, Color(200, 50, 50));
  pens[STYLE_RED].Create(width_normal, Color(200, 50, 50));

  pens[STYLE_GREENDASH].Create(Pen::DASH2, width_normal, COLOR_GREEN);
  pens[STYLE_GREEN].Create(width_normal, COLOR_GREEN);

  pens[STYLE_BLACK].Create(width_normal, COLOR_BLACK);
  pens[STYLE_WHITE].Create(width_normal, COLOR_WHITE);
  pens[STYLE_GRID].Create(Pen::DASH1, 1, Color(0xB0, 0xB0, 0xB0));
  pens[STYLE_GRIDMINOR].Create(1, Color(0xB0, 0xB0, 0xB0));
  pens[STYLE_GRIDZERO].Create(width_normal, Color(0xB0, 0xB0, 0xB0));

  bar_brush.Create(COLOR_GREEN);
  neg_brush.Create(COLOR_RED);

  label_blank_brush.Create(ColorWithAlpha(COLOR_WHITE,0xC0));
  blank_brush.Create(ColorWithAlpha(LightColor(COLOR_GRAY),0x80));
  black_brush.Create(COLOR_BLACK);

  label_font.Load(FontDescription(Layout::FontScale(12)));
  axis_label_font.Load(FontDescription(Layout::FontScale(10), true));
  axis_value_font.Load(FontDescription(Layout::FontScale(9)));

  color_positive = Color(0xa0, 0xd0, 0xf3);
  color_negative = Color(0xf3, 0xd0, 0xa0);
}
