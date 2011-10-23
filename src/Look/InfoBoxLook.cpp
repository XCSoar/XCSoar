/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "InfoBoxLook.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Layout.hpp"

#include <algorithm>

#define COLOR_INVERSE_RED Color(0xff, 0x70, 0x70)
#define COLOR_INVERSE_BLUE Color(0x90, 0x90, 0xff)
#define COLOR_INVERSE_YELLOW COLOR_YELLOW
#define COLOR_INVERSE_GREEN COLOR_GREEN
#define COLOR_INVERSE_MAGENTA COLOR_MAGENTA

void
InfoBoxLook::Initialise(bool _inverse, bool use_colors)
{
  inverse = _inverse;

  value.fg_color = title.fg_color = comment.fg_color =
    inverse ? COLOR_WHITE : COLOR_BLACK;
  background_brush.Set(inverse ? COLOR_BLACK : COLOR_WHITE);

  Color border_color = Color(128, 128, 128);
  border_pen.set(BORDER_WIDTH, border_color);
  selector_pen.set(Layout::Scale(1) + 2, value.fg_color);

  value.font = &Fonts::InfoBox;
  title.font = &Fonts::Title;
  comment.font = &Fonts::Title;
  small_font = &Fonts::InfoBoxSmall;

  colors[0] = border_color;
  if (use_colors) {
    colors[1] = inverse ? COLOR_INVERSE_RED : COLOR_RED;
    colors[2] = inverse ? COLOR_INVERSE_BLUE : COLOR_BLUE;
    colors[3] = inverse ? COLOR_INVERSE_GREEN : COLOR_GREEN;
    colors[4] = inverse ? COLOR_INVERSE_YELLOW : COLOR_YELLOW;
    colors[5] = inverse ? COLOR_INVERSE_MAGENTA : COLOR_MAGENTA;
  } else
    std::fill(colors + 1, colors + 6, border_color);
}
