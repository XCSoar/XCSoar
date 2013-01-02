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

#include "DialogLook.hpp"
#include "Screen/Layout.hpp"

#ifdef EYE_CANDY
#include "resource.h"
#endif

void
DialogLook::Initialise(const Font &caption_font,
                       const Font &_text_font,
                       const Font &_small_font,
                       const Font &button_font,
                       const Font &list_font)
{
  caption.text_color = COLOR_BLACK;
  caption.font = &caption_font;

#ifdef EYE_CANDY
  caption.background_bitmap.Load(IDB_DIALOGTITLE);
#else
  caption.background_color = COLOR_XCSOAR_DARK;
  caption.inactive_background_color = COLOR_GRAY;
#endif

  SetBackgroundColor(Color(0xe2, 0xdc, 0xbe));
  text_color = COLOR_BLACK;

  text_font = &_text_font;
  small_font = &_small_font;
  button.Initialise(button_font);

  focused.background_color = COLOR_XCSOAR_DARK;
  focused.text_color = COLOR_WHITE;
  focused.border_pen.Set(Layout::FastScale(1) + 2, COLOR_BLACK);

  list.background_color = COLOR_WHITE;
  list.text_color = COLOR_BLACK;
  list.selected.background_color = COLOR_XCSOAR_LIGHT;
  list.selected.text_color = COLOR_BLACK;
  list.focused.background_color = COLOR_XCSOAR;
  list.focused.text_color = COLOR_WHITE;
  list.pressed.background_color = COLOR_YELLOW;
  list.pressed.text_color = COLOR_BLACK;
  list.font = &list_font;
}

void
DialogLook::SetBackgroundColor(Color color)
{
  background_color = color;
  background_brush.Set(color);
}
