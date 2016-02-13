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

#include "VarioLook.hpp"
#include "FontDescription.hpp"
#include "Screen/Layout.hpp"
#include "Units/Units.hpp"
#include "Resources.hpp"

#include <algorithm>

void
VarioLook::Initialise(bool _inverse, bool _colors,
                      const Font &_text_font)
{
  inverse = _inverse;
  colors = _colors;

  if (inverse) {
    background_color = COLOR_BLACK;
    text_color = COLOR_WHITE;
    dimmed_text_color = Color(0xa0, 0xa0, 0xa0);
    sink_color = Color(0xc4, 0x80, 0x1e);
    lift_color = Color(0x1e, 0xf1, 0x73);
  } else {
    background_color = COLOR_WHITE;
    text_color = COLOR_BLACK;
    dimmed_text_color = Color((uint8_t)~0xa0, (uint8_t)~0xa0, (uint8_t)~0xa0);
    sink_color = Color(0xa3,0x69,0x0d);
    lift_color = Color(0x19,0x94,0x03);
  }

  sink_brush.Create(sink_color);
  lift_brush.Create(lift_color);

  thick_background_pen.Create(Layout::Scale(5), background_color);
  thick_sink_pen.Create(Layout::Scale(5), sink_color);
  thick_lift_pen.Create(Layout::Scale(5), lift_color);

  background_bitmap.Load(Units::GetUserVerticalSpeedUnit() == Unit::KNOTS
                         ? IDB_VARIOSCALEC : IDB_VARIOSCALEA);
  background_x = inverse ? 58 : 0;

  climb_bitmap.Load(inverse ? IDB_CLIMBSMALLINV : IDB_CLIMBSMALL);

  text_font = &_text_font;

  const unsigned value_font_height = Layout::FontScale(10);
  value_font.Load(FontDescription(value_font_height, false, false, true));

  unsigned unit_font_height = std::max(value_font_height * 2u / 5u, 7u);
  unit_font.Load(FontDescription(unit_font_height));
  unit_fraction_pen.Create(1, COLOR_GRAY);
}
