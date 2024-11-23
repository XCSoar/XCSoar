// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NavigatorLook.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Color.hpp"
#include "ui/canvas/Pen.hpp"
#include "ui/canvas/custom/Cache.hpp"

void
NavigatorLook::Initialise(bool _inverse)
{
  // font_map = &_map_font;

  inverse = _inverse;

  Color pen_frame_color, brush_frame_color;

  if (!inverse) {
    pen_frame_color = color_frame;
    brush_frame_color = color_background_frame;
  } else {
    pen_frame_color = color_frame_inv;
    brush_frame_color = color_background_frame_inv;
  }

  pen_frame.Create(Layout::ScaleFinePenWidth(1), pen_frame_color);
  brush_frame.Create(brush_frame_color);

  // aircraft_pen.Create(Layout::Scale(2), COLOR_BLACK);

  // sky_brush.Create(sky_color);
  // sky_pen.Create(Layout::Scale(1), DarkColor(sky_color));

  // terrain_brush.Create(terrain_color);
  // terrain_pen.Create(Layout::Scale(1), COLOR_GRAY);

  // ReinitialiseLayout(width, scale_title_font);
}

// void
// NavigatorLook::ReinitialiseLayout(unsigned width, unsigned scale_title_font)
// {
// FontDescription title_font_d(8);
// AutoSizeFont(title_font_d, (width * scale_title_font) / 100U,
//              _T("1234567890A"));

// title_font.Load(title_font_d);

// FontDescription value_font_d(10, true);
// AutoSizeFont(value_font_d, width, _T("1234m"));
// value_font.Load(value_font_d);

// FontDescription small_value_font_d(10);
// AutoSizeFont(small_value_font_d, width, _T("12345m"));
// small_value_font.Load(small_value_font_d);

// unsigned unit_font_height = std::max(value_font_d.GetHeight() * 2u / 5u,
// 7u); unit_font.Load(FontDescription(unit_font_height));

// #ifdef HAVE_TEXT_CACHE
//   TextCache::Flush();
// #endif
// }