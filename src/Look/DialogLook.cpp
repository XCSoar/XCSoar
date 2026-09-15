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

    /* a dithered display has no shade below the page: in dark mode
       the page is the black level itself */
    SetBackgroundColor(IsDithered()
                       ? COLOR_BLACK
                       : COLOR_DARK_THEME_BACKGROUND);
    text_color = COLOR_WHITE;

    focused.background_color = COLOR_XCSOAR;
    focused.text_color = COLOR_WHITE;
    focused.border_pen.Create(Layout::FastScale(1) + 2, COLOR_WHITE);

    list.background_color = IsDithered()
      ? COLOR_BLACK
      : COLOR_DARK_THEME_LIST;
    list.text_color = COLOR_WHITE;
    list.selected.background_color = IsDithered()
      ? COLOR_DARK_GRAY
      : COLOR_DARK_THEME_LIST_SELECTED;
    list.selected.text_color = COLOR_WHITE;
    list.focused.background_color = IsDithered()
      ? COLOR_WHITE
      : COLOR_XCSOAR_DARK;
    list.focused.text_color = IsDithered() ? COLOR_BLACK : COLOR_WHITE;
    list.pressed.background_color = DarkColor(COLOR_YELLOW);
    list.pressed.text_color = COLOR_WHITE;
  } else {
    caption.text_color = COLOR_BLACK;
    caption.background_color = IsDithered() ? COLOR_BLACK : COLOR_XCSOAR_DARK;
    caption.inactive_background_color = COLOR_GRAY;

    SetBackgroundColor(IsDithered() ? COLOR_WHITE : COLOR_DIALOG_BACKGROUND);
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

  /* #ButtonState::SELECTED (action bar) matches a focused list row; in dark
     mode use the same bright `focused` (COLOR_XCSOAR) as dialog/tab focus
     list.focused in dark is darker (XCSOAR_DARK) */
  {
    if (dark_mode && IsDithered()) {
      /* the level the dark page does not use */
      button.selected.background_color = COLOR_WHITE;
      button.selected.foreground_color = COLOR_BLACK;
    } else if (dark_mode && !HasColors()) {
      /* the mirror of the light mode: one step further from the
         button face than the focused card */
      button.selected.background_color =
        MixColors(COLOR_WHITE, button.standard.background_color, 0x50);
      button.selected.foreground_color = COLOR_WHITE;
    } else if (dark_mode) {
      button.selected.background_color = focused.background_color;
      button.selected.foreground_color = focused.text_color;
    } else if (!HasColors() && !IsDithered()) {
      /* without a hue the states line up on the lightness scale, one
         step further from the white enabled face with every one; a
         dithered display has no such scale */
      button.selected.background_color = COLOR_BUTTON_RING;
      button.selected.foreground_color = COLOR_BLACK;
    } else {
      const auto &h = list.focused;
      button.selected.background_color = h.background_color;
      button.selected.foreground_color = h.text_color;
    }
    button.selected.foreground_brush.Create(button.selected.foreground_color);
    /* pressed goes down the primary scale; the state is told apart
       from the focused one by #ButtonLook::selected_ring_color,
       not by a border on the face */
    button.selected.pressed_background_color =
      HasColors() && !IsDithered()
      ? COLOR_XCSOAR_PRESSED
      : MixColors(dark_mode && !IsDithered() ? COLOR_WHITE : COLOR_BLACK,
                  button.selected.background_color, 0x40);
    /* solid faces have no border ring in Nuxt UI */
    button.selected.ring_color = button.selected.background_color;
  }

  check_box.Initialise(text_font, dark_mode);

  list.font = &text_font;
  list.font_bold = &bold_font;
}

void
DialogLook::SetBackgroundColor(Color color)
{
  background_color = color;
  background_brush.Create(color);
}

Color
DialogLook::ReadOnlyValueBackground() const noexcept
{
  return dark_mode ? DarkColor(list.background_color) : Color(0xf0, 0xf0, 0xf0);
}

Color
DialogLook::ReadOnlyValueBorderColor() const noexcept
{
  return dark_mode ? COLOR_GRAY : COLOR_BLACK;
}
