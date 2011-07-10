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
#include "Screen/Graphics.hpp"

void
InfoBoxLook::Initialise(bool inverse)
{
  value.fg_color = title.fg_color = comment.fg_color =
    inverse ? COLOR_WHITE : COLOR_BLACK;
  background_brush.set(inverse ? COLOR_BLACK : COLOR_WHITE);

  Color border_color = Color(128, 128, 128);
  border_pen.set(BORDER_WIDTH, border_color);
  selector_pen.set(IBLSCALE(1) + 2, value.fg_color);

  value.font = &Fonts::InfoBox;
  title.font = &Fonts::Title;
  comment.font = &Fonts::Title;
  small_font = &Fonts::InfoBoxSmall;

  colors[0] = border_color;
  colors[1] = inverse ? Graphics::inv_redColor : COLOR_RED;
  colors[2] = inverse ? Graphics::inv_blueColor : COLOR_BLUE;
  colors[3] = inverse ? Graphics::inv_greenColor : COLOR_GREEN;
  colors[4] = inverse ? Graphics::inv_yellowColor : COLOR_YELLOW;
  colors[5] = inverse ? Graphics::inv_magentaColor : COLOR_MAGENTA;
}
