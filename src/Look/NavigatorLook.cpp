// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NavigatorLook.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Color.hpp"
#include "ui/canvas/Pen.hpp"

void
NavigatorLook::Initialise(bool _inverse, const Font &_font)
{
  font = &_font;

  Color pen_frame_color, brush_frame_color;

  if(!_inverse) {
    pen_frame_color = frame_color;
    brush_frame_color = background_color;
  }
  else {
    pen_frame_color = frame_color_inv;
    brush_frame_color = background_color_inv;
  }

  frame_brush.Create(pen_frame_color);
  frame_pen.Create(Layout::ScalePenWidth(1), pen_frame_color);
  
  background_brush.Create(brush_frame_color);
  background_pen.Create(Layout::ScalePenWidth(1), brush_frame_color);

  aircraft_pen.Create(Layout::Scale(2), COLOR_BLACK);

  sky_brush.Create(sky_color);
  sky_pen.Create(Layout::Scale(1), DarkColor(sky_color));

  terrain_brush.Create(terrain_color);
  terrain_pen.Create(Layout::Scale(1), COLOR_GRAY);
}
