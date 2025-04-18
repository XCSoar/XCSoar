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
}

void
DialogLook::Initialise()
{
  LoadFonts();

  caption.text_color = COLOR_BLACK;
  caption.font = &text_font;

#ifdef EYE_CANDY
  caption.background_bitmap.Load(IDB_DIALOGTITLE);
#endif

  caption.background_color = IsDithered() ? COLOR_BLACK : COLOR_XCSOAR_DARK;
  caption.inactive_background_color = COLOR_GRAY;

  if (IsDithered())
    SetBackgroundColor(COLOR_WHITE);
  else
    SetBackgroundColor(Color(0xe2, 0xdc, 0xbe));
  text_color = COLOR_BLACK;

  button.Initialise(bold_font);
  check_box.Initialise(text_font);

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
  list.font = &text_font;
  list.font_bold = &bold_font;
}

void
DialogLook::SetBackgroundColor(Color color)
{
  background_color = color;
  background_brush.Create(color);
}
