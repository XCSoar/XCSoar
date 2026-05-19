// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NavigatorLook.hpp"
#include "Screen/Layout.hpp"

void
NavigatorLook::Initialise(bool _inverse)
{
  Color pen_frame_color, brush_frame_color;

  if (!(inverse = _inverse)) {
    pen_frame_color = color_frame;
    brush_frame_color = color_background_frame;
  } else {
    pen_frame_color = color_frame_inv;
    brush_frame_color = color_background_frame_inv;
  }
  pen_frame.Create(Layout::ScaleFinePenWidth(1), pen_frame_color);
  brush_frame.Create(brush_frame_color);
}
