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

#include "ButtonLook.hpp"
#include "Asset.hpp"

void
ButtonLook::Initialise(const Font &_font)
{
  font = &_font;

  standard.foreground_color = COLOR_BLACK;
  standard.foreground_brush.Set(standard.foreground_color);
  standard.background_color = COLOR_LIGHT_GRAY;
  if (!HasColors()) {
    standard.light_border_pen.Set(1, LightColor(COLOR_DARK_GRAY));
    standard.dark_border_pen.Set(1, COLOR_BLACK);
  } else {
    standard.light_border_pen.Set(1, LightColor(standard.background_color));
    standard.dark_border_pen.Set(1, DarkColor(standard.background_color));
  }

  focused.foreground_color = COLOR_WHITE;
  focused.foreground_brush.Set(focused.foreground_color);
  focused.background_color = COLOR_XCSOAR_DARK;
  if (!HasColors()) {
    focused.light_border_pen.Set(1, LightColor(COLOR_DARK_GRAY));
    focused.dark_border_pen.Set(1, COLOR_BLACK);
  } else {
    focused.light_border_pen.Set(1, LightColor(focused.background_color));
    focused.dark_border_pen.Set(1, DarkColor(focused.background_color));
  }

  disabled.color = COLOR_GRAY;
  disabled.brush.Set(disabled.color);
}
