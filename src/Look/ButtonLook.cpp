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

#include "ButtonLook.hpp"
#include "Colors.hpp"
#include "Asset.hpp"

#include "Look/Themes/BlueTheme.hpp"

void ButtonLook::Initialise(const Font &_font)
{
  font = &_font;

  standard.foreground_color = COLOR_BUTTON_NORMAL_FOREGROUND;
  standard.background_color = COLOR_BUTTON_NORMAL_BACKGROUND_LIGHT;
  standard.background_color2 = COLOR_BUTTON_NORMAL_BACKGROUND_DARK;
  standard.foreground_brush.Create(COLOR_BUTTON_NORMAL_FOREGROUND);

  focused.foreground_color = COLOR_BUTTON_FOCUSSED_FOREGROUND;
  focused.background_color = COLOR_BUTTON_FOCUSSED_BACKGROUND_LIGHT;
  focused.background_color2 = COLOR_BUTTON_FOCUSSED_BACKGROUND_DARK;
  focused.foreground_brush.Create(COLOR_BUTTON_FOCUSSED_FOREGROUND);

  pressed.foreground_color = COLOR_BUTTON_PRESSED_FOREGROUND;
  pressed.background_color = COLOR_BUTTON_PRESSED_BACKGROUND_LIGHT;
  pressed.background_color2 = COLOR_BUTTON_PRESSED_BACKGROUND_DARK;
  pressed.foreground_brush.Create(COLOR_BUTTON_PRESSED_FOREGROUND);

  disabled.foreground_color = COLOR_BUTTON_DISABLED_FOREGROUND;
  disabled.background_color = COLOR_BUTTON_DISABLED_BACKGROUND_LIGHT;
  disabled.background_color2 = COLOR_BUTTON_DISABLED_BACKGROUND_DARK;
  disabled.foreground_brush.Create(COLOR_BUTTON_DISABLED_FOREGROUND);
}
