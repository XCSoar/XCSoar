// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OverlayLook.hpp"
#include "Screen/Layout.hpp"
#include "Resources.hpp"

void
OverlayLook::Initialise([[maybe_unused]] const Font &font, const Font &bold_font)
{
  map_scale_left_icon.LoadResource(IDB_MAPSCALE_LEFT, IDB_MAPSCALE_LEFT_HD, false);
  map_scale_right_icon.LoadResource(IDB_MAPSCALE_RIGHT, IDB_MAPSCALE_RIGHT_HD, false);
  overlay_font = &bold_font;
  crosshair_pen.Create(Pen::SOLID, Layout::ScalePenWidth(2), COLOR_DARK_GRAY);
  crosshair_pen_alias.Create(Pen::SOLID, Layout::ScalePenWidth(2), COLOR_WHITE);
}
