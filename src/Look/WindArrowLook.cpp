// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WindArrowLook.hpp"
#include "Screen/Layout.hpp"
#include "Asset.hpp"
#include "Look/Colors.hpp"

void
WindArrowLook::Initialise(const Font &_font, bool inverse)
{
  arrow_pen.Create(Layout::ScalePenWidth(1),
                   inverse
                   ? (HasColors() ? LightColor(COLOR_GRAY) : COLOR_WHITE)
                   : (HasColors() ? DarkColor(COLOR_GRAY) : COLOR_BLACK));
  shaft_pen.Create(Pen::DASH2, Layout::ScalePenWidth(1), inverse ? COLOR_WHITE : COLOR_BLACK);
  arrow_brush.Create(IsDithered() ? COLOR_DARK_GRAY : ColorWithAlpha(COLOR_GRAY, ALPHA_OVERLAY));

  font = &_font;
}
