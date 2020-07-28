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

#include "DialogLook.hpp"
#include "FontDescription.hpp"
#include "Colors.hpp"
#include "Screen/Layout.hpp"
#include "Asset.hpp"
#include "Look/Themes/BlueTheme.hpp"

#ifdef EYE_CANDY
#include "Resources.hpp"
#endif

#include <algorithm>

void DialogLook::Initialise()
{
  const FontDescription text_font_d(std::min(Layout::FontScale(12),
                                             Layout::min_screen_pixels / 20));
  const FontDescription small_font_d =
      text_font_d.WithHeight(text_font_d.GetHeight() * 3u / 4u);

  text_font.Load(text_font_d);
  small_font.Load(small_font_d);

  bold_font.Load(text_font_d.WithBold());

  caption.text_color = COLOR_DIALOG_CAPTION_FOREGROUND;
  caption.font = &text_font;

  caption.background_color = IsDithered() ? COLOR_BLACK : COLOR_DIALOG_CAPTION_BACKGROUND;
  caption.inactive_background_color = COLOR_DIALOG_INACTIVE_BACKGROUND;

  SetBackgroundColor(COLOR_DIALOG_BACKGROUND);
  text_color = COLOR_DIALOG_FOREGROUND;

  button.Initialise(bold_font);
  check_box.Initialise(text_font);

  focused.background_color = COLOR_XCSOAR_DARK;
  focused.text_color = COLOR_WHITE;
  focused.border_pen.Create(Layout::FastScale(1) + 2, COLOR_BLACK);

  list.background_color = COLOR_LIST_BACKGROUND;
  list.text_color = COLOR_LIST_FOREGROUND;
  list.selected.background_color = IsDithered()
                                       ? COLOR_VERY_LIGHT_GRAY
                                       : COLOR_LIST_SELECTED_BACKGROUND;
  list.selected.text_color = COLOR_LIST_SELECTED_FOREGROUND;
  list.focused.background_color = IsDithered() ? COLOR_BLACK : COLOR_LIST_FOCUSED_BACKGROUND;
  list.focused.text_color = COLOR_LIST_FOCUSED_FOREGROUND;
  list.pressed.background_color = COLOR_LIST_PRESSED_BACKGROUND;
  list.pressed.text_color = COLOR_LIST_PRESSED_FOREGROUND;
  list.font = &text_font;
  list.font_bold = &bold_font;
}

void DialogLook::SetBackgroundColor(Color color)
{
  background_color = color;
  background_brush.Create(color);
}
