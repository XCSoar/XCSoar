// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DialogLook.hpp"
#include "FontDescription.hpp"
#include "Colors.hpp"
#include "Screen/Layout.hpp"
#include "Asset.hpp"

#ifdef EYE_CANDY
#include "Resources.hpp"
#endif

#include <algorithm>

void
DialogLook::LoadFonts()
{
  const FontDescription text_font_d(std::min(Layout::FontScale(12),
                                             Layout::min_screen_pixels / 20));
  const FontDescription small_font_d =
    text_font_d.WithHeight(text_font_d.GetHeight() * 3u / 4u);

  text_font.Load(text_font_d);
  small_font.Load(small_font_d);

  bold_font.Load(text_font_d.WithBold());

  /* H1 = 150% of text size, H2 = 125% */
  heading1_font.Load(text_font_d.WithHeight(text_font_d.GetHeight() * 3u / 2u).WithBold());
  heading2_font.Load(text_font_d.WithHeight(text_font_d.GetHeight() * 5u / 4u).WithBold());
}

void
DialogLook::Initialise(bool _dark_mode)
{
  dark_mode = _dark_mode;

  LoadFonts();

  caption.font = &text_font;

#ifdef EYE_CANDY
  caption.background_bitmap.Load(IDB_DIALOGTITLE);
#endif

  if (dark_mode) {
    caption.text_color = COLOR_WHITE;
    caption.background_color = COLOR_DARK_THEME_CAPTION;
    caption.inactive_background_color = COLOR_DARK_THEME_CAPTION_INACTIVE;

    SetBackgroundColor(COLOR_DARK_THEME_BACKGROUND);
    /* subtle gradient: lighter at top, base color at bottom */
    background_gradient_top_color = Color(0x14, 0x22, 0x32);
    text_color = COLOR_WHITE;

    focused.background_color = COLOR_XCSOAR;
    focused.text_color = COLOR_WHITE;
    focused.border_pen.Create(Layout::FastScale(1) + 2, COLOR_WHITE);

    list.background_color = COLOR_DARK_THEME_LIST;
    list.text_color = COLOR_WHITE;
    list.selected.background_color = COLOR_DARK_THEME_LIST_SELECTED;
    list.selected.text_color = COLOR_WHITE;
    list.focused.background_color = COLOR_XCSOAR_DARK;
    list.focused.text_color = COLOR_WHITE;
    list.pressed.background_color = DarkColor(COLOR_YELLOW);
    list.pressed.text_color = COLOR_WHITE;
  } else {
    caption.text_color = COLOR_BLACK;
    caption.background_color = IsDithered() ? COLOR_BLACK : COLOR_XCSOAR_DARK;
    caption.inactive_background_color = COLOR_GRAY;

    if (IsDithered())
      SetBackgroundColor(COLOR_WHITE);
    else {
      SetBackgroundColor(Color(0xe2, 0xdc, 0xbe));
      /* subtle gradient: lighter at top, base color at bottom */
      background_gradient_top_color = Color(0xf0, 0xeb, 0xd4);
    }
    text_color = COLOR_BLACK;

    focused.background_color = COLOR_XCSOAR_DARK;
    focused.text_color = COLOR_WHITE;
    focused.border_pen.Create(Layout::FastScale(1) + 2, COLOR_BLACK);

    list.background_color = COLOR_WHITE;
    list.text_color = COLOR_BLACK;
    list.selected.background_color = IsDithered()
      ? COLOR_VERY_LIGHT_GRAY : COLOR_XCSOAR_LIGHT;
    list.selected.text_color = COLOR_BLACK;
    list.focused.background_color = IsDithered() ? COLOR_BLACK : COLOR_XCSOAR;
    list.focused.text_color = COLOR_WHITE;
    list.pressed.background_color = COLOR_YELLOW;
    list.pressed.text_color = COLOR_BLACK;
  }

  button.Initialise(bold_font, dark_mode);
  check_box.Initialise(text_font, dark_mode);

  list.font = &text_font;
  list.font_bold = &bold_font;
}

void
DialogLook::SetBackgroundColor(Color color)
{
  background_color = color;
  background_gradient_top_color = color;
  background_brush.Create(color);
}
