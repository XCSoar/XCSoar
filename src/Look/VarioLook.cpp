// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VarioLook.hpp"
#include "FontDescription.hpp"
#include "Screen/Layout.hpp"
#include "Units/Units.hpp"
#include "Resources.hpp"
#include "ui/canvas/Features.hpp" // for HAVE_TEXT_CACHE

#ifdef HAVE_TEXT_CACHE
#include "ui/canvas/custom/Cache.hpp"
#endif

#include <algorithm>

void
VarioLook::Initialise(bool _inverse, bool _colors,
                      unsigned width,
                      const Font &_text_font)
{
  inverse = _inverse;
  colors = _colors;

  if (inverse) {
    background_color = COLOR_BLACK;
    text_color = COLOR_WHITE;
    dimmed_text_color = Color(0xa0, 0xa0, 0xa0);
    sink_color = Color(0xc4, 0x80, 0x1e);
    lift_color = Color(0x1e, 0xf1, 0x73);
  } else {
    background_color = COLOR_WHITE;
    text_color = COLOR_BLACK;
    dimmed_text_color = Color((uint8_t)~0xa0, (uint8_t)~0xa0, (uint8_t)~0xa0);
    sink_color = Color(0xa3,0x69,0x0d);
    lift_color = Color(0x19,0x94,0x03);
  }

  sink_brush.Create(sink_color);
  lift_brush.Create(lift_color);

  arc_pen.Create(Layout::ScalePenWidth(2), text_color);
  tick_pen.Create(Layout::ScalePenWidth(1), text_color);

  thick_background_pen.Create(Layout::Scale(5), background_color);
  thick_sink_pen.Create(Layout::Scale(5), sink_color);
  thick_lift_pen.Create(Layout::Scale(5), lift_color);

  climb_bitmap.Load(inverse ? IDB_CLIMBSMALLINV : IDB_CLIMBSMALL);

  text_font = &_text_font;

  ReinitialiseLayout(width);
}

void
VarioLook::ReinitialiseLayout(unsigned width)
{
  /* Layout::FontScale() applies the configured UI scale, and
     additionally we limit font sizes if the vario gauge is small */

  const unsigned arc_label_font_height = std::min(Layout::FontScale(14), width / 5);
  arc_label_font.Load(FontDescription{arc_label_font_height, true});

  const unsigned value_font_height = std::min(Layout::FontScale(10), width / 6);
  value_font.Load(FontDescription(value_font_height, false, false, true));

  unsigned unit_font_height = std::max(value_font_height * 2u / 5u, 7u);
  unit_font.Load(FontDescription(unit_font_height));
  unit_fraction_pen.Create(1, COLOR_GRAY);

#ifdef HAVE_TEXT_CACHE
  TextCache::Flush();
#endif
}
