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

#include "InfoBoxLook.hpp"
#include "FontDescription.hpp"
#include "Colors.hpp"
#include "Screen/Layout.hpp"
#include "AutoFont.hpp"

#ifdef HAVE_TEXT_CACHE
#include "Screen/Custom/Cache.hpp"
#endif

#include <algorithm>

#define COLOR_INVERSE_RED Color(0xff, 0x70, 0x70)
#define COLOR_INVERSE_BLUE Color(0x90, 0x90, 0xff)
#define COLOR_INVERSE_YELLOW COLOR_YELLOW
#define COLOR_INVERSE_GREEN COLOR_GREEN
#define COLOR_INVERSE_MAGENTA COLOR_MAGENTA

void
InfoBoxLook::Initialise(bool _inverse, bool use_colors,
                        unsigned width)
{
  inverse = _inverse;

  value.fg_color = title.fg_color = comment.fg_color =
    inverse ? COLOR_WHITE : COLOR_BLACK;
  background_color = inverse ? COLOR_BLACK : COLOR_WHITE;
  caption_background_color = inverse
    ? Color(0x40, 0x40, 0x40)
    : Color(0xe0, 0xe0, 0xe0);
  focused_background_color = COLOR_XCSOAR_LIGHT;
  pressed_background_color = COLOR_YELLOW;

  Color border_color = Color(128, 128, 128);
  border_pen.Create(BORDER_WIDTH, border_color);

  ReinitialiseLayout(width);

  unit_fraction_pen.Create(1, value.fg_color);

  colors[0] = border_color;
  if (HasColors() && use_colors) {
    colors[1] = inverse ? COLOR_INVERSE_RED : COLOR_RED;
    colors[2] = inverse ? COLOR_INVERSE_BLUE : COLOR_BLUE;
    colors[3] = inverse ? COLOR_INVERSE_GREEN : Color(0, 192, 0);
    colors[4] = inverse ? COLOR_INVERSE_YELLOW : COLOR_YELLOW;
    colors[5] = inverse ? COLOR_INVERSE_MAGENTA : COLOR_MAGENTA;
  } else
    std::fill(colors + 1, colors + 6, inverse ? COLOR_WHITE : COLOR_BLACK);
}

void
InfoBoxLook::ReinitialiseLayout(unsigned width)
{
  FontDescription title_font_d(8);
  AutoSizeFont(title_font_d, width, _T("123456789012345"));
  title_font.Load(title_font_d);

  FontDescription value_font_d(10, true);
  AutoSizeFont(value_font_d, width, _T("1234m"));
  value_font.Load(value_font_d);

  FontDescription small_value_font_d(10);
  AutoSizeFont(small_value_font_d, width, _T("12345m"));
  small_value_font.Load(small_value_font_d);

  unsigned unit_font_height = std::max(value_font_d.GetHeight() * 2u / 5u, 7u);
  unit_font.Load(FontDescription(unit_font_height));

#ifdef HAVE_TEXT_CACHE
  TextCache::Flush();
#endif
}
