// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxLook.hpp"
#include "FontDescription.hpp"
#include "Colors.hpp"
#include "ui/canvas/Features.hpp" // for HAVE_TEXT_CACHE
#include "Screen/Layout.hpp"
#include "AutoFont.hpp"
#include "Asset.hpp"
#include "ui/canvas/Color.hpp"

#ifdef HAVE_TEXT_CACHE
#include "ui/canvas/custom/Cache.hpp"
#endif

#include <algorithm>


void
InfoBoxLook::Initialise(bool _inverse, bool use_colors,
                        unsigned width, unsigned scale_title_font)
{
  inverse = _inverse;

  value.fg_color = title.fg_color = comment.fg_color =
    inverse ? COLOR_WHITE : COLOR_BLACK;
  background_color = inverse ? COLOR_BLACK : COLOR_WHITE;
  caption_background_color = inverse
    ? Color(0x40, 0x40, 0x40)
    : Color(0xe0, 0xe0, 0xe0);
  focused_background_color = COLOR_XCSOAR_LIGHT;
  pressed_background_color = COLOR_YELLOW;
  /* the arrange backdrop is the dialog background; on e-paper the
     XCSoar fill becomes a mid-tone, so use the opposite of the
     card so the text stays black-on-white or the reverse */
  preview_active_color = HasColors()
    ? COLOR_XCSOAR
    : (inverse ? COLOR_WHITE : COLOR_BLACK);
  /* same COLOR_GRAY as border_pen; white on dark is focus-level
     contrast and NASA reserves that for important items */
  preview_border_color = inverse ? COLOR_GRAY : COLOR_BLACK;

  if (inverse) {
    focused_background_color = DarkColor(focused_background_color);
    pressed_background_color = DarkColor(pressed_background_color);
  }

  ReinitialiseLayout(width, scale_title_font);

  colors[0] = COLOR_GRAY;
  if (HasColors() && use_colors) {
    colors[1] = inverse ? COLOR_INVERSE_RED : COLOR_RED;
    colors[2] = inverse ? COLOR_INVERSE_BLUE : COLOR_BLUE;
    colors[3] = inverse ? COLOR_INVERSE_GREEN : COLOR_LIGHT_GREEN;
    colors[4] = inverse ? COLOR_INVERSE_YELLOW : COLOR_AMBER;
    colors[5] = inverse ? COLOR_INVERSE_MAGENTA : COLOR_MAGENTA;
  } else
    std::fill(colors + 1, colors + 6, inverse ? COLOR_WHITE : COLOR_BLACK);
}

void
InfoBoxLook::ReinitialiseLayout(unsigned width, unsigned scale_title_font)
{
  border_width = Layout::ScaleFinePenWidth(1);

  Color border_color = COLOR_GRAY;
  border_pen.Create(border_width, border_color);
  /* one device pixel; ScaleFinePenWidth(1) is the live InfoBox
     chrome and looks like a frame on a tablet */
  preview_border_width = 1;
  /* halo outside the card hairline; filled, not a thick stroke */
  preview_focus_width = Layout::ScalePenWidth(1);

  preview_padding = Layout::Scale(4);
  preview_radius = Layout::Scale(6);

  unit_fraction_pen.Create(Layout::ScaleFinePenWidth(1), value.fg_color);

  FontDescription title_font_d(8);
  AutoSizeFont(title_font_d, (width * scale_title_font) / 100U,
               "1234567890A");

  title_font.Load(title_font_d);
  title_font_bold.Load(title_font_d.WithBold(true));

  preview_number_font.Load(FontDescription(std::max(title_font_d.GetHeight()
                                                    * 2u / 3u, 7u)));

  FontDescription value_font_d(10, true);
  AutoSizeFont(value_font_d, width, "1234m");
  value_font.Load(value_font_d);

  FontDescription small_value_font_d(10);
  AutoSizeFont(small_value_font_d, width, "12345m");
  small_value_font.Load(small_value_font_d);

  unsigned unit_font_height = std::max(value_font_d.GetHeight() * 2u / 5u, 7u);
  unit_font.Load(FontDescription(unit_font_height));

#ifdef HAVE_TEXT_CACHE
  TextCache::Flush();
#endif
}

Color
InfoBoxLook::GetPreviewGlowColor() const noexcept
{
  return HasColors()
    ? LightColor(preview_active_color)
    : title.fg_color;
}
