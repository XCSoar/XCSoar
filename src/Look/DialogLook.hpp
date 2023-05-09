// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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

  void LoadFonts();

  void Initialise();

  void SetBackgroundColor(Color color);
};
