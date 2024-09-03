// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ThermalAssistantLook.hpp"
#include "FontDescription.hpp"
#include "Screen/Layout.hpp"

void
ThermalAssistantLook::Initialise(bool small, bool inverse)
{
  background_color = inverse? COLOR_BLACK: COLOR_WHITE;
  text_color = inverse? COLOR_WHITE: COLOR_BLACK;
#ifdef ENABLE_OPENGL
  polygon_brush.Create(polygon_fill_color.WithAlpha(128));
#else /* !OPENGL */
  polygon_brush.Create(polygon_fill_color);
#endif /* !OPENGL */

  unsigned width = Layout::FastScale(small ? 1u : 2u);
#ifdef ENABLE_OPENGL
  polygon_pen.Create(width, polygon_border_color.WithAlpha(128));
#else /* !OPENGL */
  polygon_pen.Create(width, polygon_border_color);
#endif /* !OPENGL */
  inner_circle_pen.Create(1, circle_color);
  outer_circle_pen.Create(Pen::DASH2, 1, circle_color);
  plane_pen.Create(width, inverse? COLOR_WHITE: COLOR_BLACK);

  overlay_font.Load(FontDescription(Layout::FontScale(22)));
  circle_label_font.Load(FontDescription(Layout::FontScale(10)));
}
