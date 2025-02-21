// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NextArrowLook.hpp"
#include "Screen/Layout.hpp"
#include "Asset.hpp"
#include "Look/Colors.hpp"

void
NextArrowLook::Initialise(const Font &_font, bool use_colors, bool inverse)
{
  next_arrow_pen.Create(Layout::ScalePenWidth(1),
                   inverse
                   ? (HasColors() ? (use_colors ? COLOR_INVERSE_MAGENTA : COLOR_WHITE) : COLOR_WHITE)
                   : (HasColors() ? (use_colors ? COLOR_MAGENTA : COLOR_BLACK) : COLOR_BLACK));
  next_arrow_brush.Create(IsDithered() ? COLOR_DARK_GRAY : ColorWithAlpha(COLOR_MAGENTA, ALPHA_OVERLAY));

  font = &_font;
}
