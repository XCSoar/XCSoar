/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_DIALOG_LOOK_HPP
#define XCSOAR_DIALOG_LOOK_HPP

#include "ButtonLook.hpp"
#include "CheckBoxLook.hpp"
#include "ui/canvas/Color.hpp"
#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"
#include "ui/canvas/Font.hpp"

#ifdef EYE_CANDY
#include "ui/canvas/Bitmap.hpp"
#endif

struct DialogLook {
  struct {
    Color text_color;

    const Font *font;

#ifdef EYE_CANDY
    Bitmap background_bitmap;
#endif

    Color background_color;
    Color inactive_background_color;
  } caption;

  Color background_color, text_color;

  Brush background_brush;

  Font text_font, bold_font, small_font;

  struct {
    Color background_color, text_color;

    Pen border_pen;
  } focused;

  struct {
    Color background_color, text_color;

    struct {
      Color background_color, text_color;
    } selected;

    /**
     * Colors for the selected item when the list has the keyboard
     * focus.
     */
    struct {
      Color background_color, text_color;
    } focused;

    /**
     * Colors for the selected item when the mouse/finger is pressed.
     */
    struct {
      Color background_color, text_color;
    } pressed;

    const Font *font, *font_bold;

    [[gnu::pure]]
    Color GetTextColor(bool is_selected, bool is_focused,
                       bool is_pressed) const {
      return is_pressed
        ? pressed.text_color
        : (is_selected
           ? (is_focused
              ? focused.text_color
              : selected.text_color)
           : text_color);
    }

    [[gnu::pure]]
    Color GetBackgroundColor(bool is_selected, bool is_focused,
                             bool is_pressed) const {
      return is_pressed
        ? pressed.background_color
        : (is_selected
           ? (is_focused
              ? focused.background_color
              : selected.background_color)
           : background_color);
    }
  } list;

  ButtonLook button;
  CheckBoxLook check_box;

  void Initialise();

  void SetBackgroundColor(Color color);
};

#endif
