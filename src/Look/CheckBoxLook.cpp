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

#include "CheckBoxLook.hpp"
#include "Colors.hpp"

void
CheckBoxLook::Initialise(const Font &_font)
{
  font = &_font;

  focus_background_brush.Create(COLOR_XCSOAR_DARK);

  standard.box_brush.Create(COLOR_WHITE);
  standard.box_pen.Create(1, COLOR_BLACK);
  standard.check_brush.Create(COLOR_BLACK);
  standard.text_color = COLOR_BLACK;

  focused.box_brush.Create(COLOR_WHITE);
  focused.box_pen.Create(1, COLOR_BLACK);
  focused.check_brush.Create(COLOR_BLACK);
  focused.text_color = COLOR_WHITE;

  pressed.box_brush.Create(COLOR_XCSOAR_LIGHT);
  pressed.box_pen.Create(1, COLOR_BLACK);
  pressed.check_brush.Create(COLOR_BLACK);
  pressed.text_color = COLOR_WHITE;

  disabled.box_brush.Create(COLOR_WHITE);
  disabled.box_pen.Create(1, COLOR_GRAY);
  disabled.check_brush.Create(COLOR_GRAY);
  disabled.text_color = COLOR_GRAY;
}
