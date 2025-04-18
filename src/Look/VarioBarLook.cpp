// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "VarioBarLook.hpp"
#include "Screen/Layout.hpp"
#include "Look/Colors.hpp"

void
VarioBarLook::Initialise(const Font &_font)
{
  const uint8_t alpha = ALPHA_OVERLAY;

  brush_sink.Create(ColorWithAlpha(COLOR_RED, alpha));
  brush_sink_avg.Create(ColorWithAlpha(LightColor(COLOR_RED), alpha));
  pen_sink.Create(Layout::ScalePenWidth(1), DarkColor(COLOR_RED));

  brush_climb.Create(ColorWithAlpha(COLOR_GREEN, alpha));
  brush_climb_avg.Create(ColorWithAlpha((LightColor(LightColor(COLOR_GREEN))),
                                        alpha));
  pen_climb.Create(Layout::ScalePenWidth(1), DarkColor(COLOR_GREEN));

  brush_mc.Create(ColorWithAlpha(COLOR_GRAY, alpha));
  pen_mc.Create(Layout::ScalePenWidth(1), DarkColor(COLOR_GRAY));

  font = &_font;
}
