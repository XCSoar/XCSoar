// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VarioLook.hpp"
#include "AutoFont.hpp"
#include "FontDescription.hpp"
#include "Resources.hpp"
#include "Screen/Layout.hpp"
#include "Units/Units.hpp"
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
  FontDescription arc_label_font_d(8);
  AutoSizeFont(arc_label_font_d, width / 10, _T("-5"));
  arc_label_font.Load(arc_label_font_d);

  FontDescription value_font_d(14);
  AutoSizeFont(value_font_d, width / 1.5, _T("-00.0m"));
  value_font.Load(value_font_d);

  FontDescription unit_font_d(8);
  AutoSizeFont(unit_font_d, width / 4.22, _T("00.0m"));
  unit_font.Load(unit_font_d);
  unit_fraction_pen.Create(1, COLOR_GRAY);

  FontDescription label_font_d(8);
  AutoSizeFont(label_font_d, width / 2, _T("Auto MC"));
  label_font.Load(label_font_d);

#ifdef HAVE_TEXT_CACHE
  TextCache::Flush();
#endif
}
