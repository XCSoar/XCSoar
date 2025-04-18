// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FinalGlideBarLook.hpp"
#include "Screen/Layout.hpp"
#include "Look/Colors.hpp"
#include "Asset.hpp"

void
FinalGlideBarLook::Initialise(const Font &_font)
{
  const uint8_t alpha = ALPHA_OVERLAY;

  brush_below.Create(ColorWithAlpha(COLOR_RED, ALPHA_OVERLAY));
  brush_below_mc0.Create(ColorWithAlpha(LightColor(COLOR_RED), alpha));
  pen_below.Create(Layout::ScalePenWidth(1),
                   HasColors()? DarkColor(COLOR_RED) : COLOR_BLACK);

  brush_below_landable.Create(ColorWithAlpha(COLOR_ORANGE, alpha));
  brush_below_landable_mc0.Create(ColorWithAlpha(LightColor(COLOR_ORANGE),
                                                 alpha));
  pen_below_landable.Create(Layout::ScalePenWidth(1),
                         HasColors()? DarkColor(COLOR_ORANGE) : COLOR_BLACK);

  brush_above.Create(ColorWithAlpha(COLOR_GREEN, alpha));
  brush_above_mc0.Create(ColorWithAlpha(LightColor(LightColor(COLOR_GREEN)),
                                        alpha));
  pen_above.Create(Layout::ScalePenWidth(1),
                   HasColors() ? DarkColor(COLOR_GREEN) : COLOR_BLACK);

  font = &_font;
}
