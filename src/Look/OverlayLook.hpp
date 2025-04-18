// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Icon.hpp"
#include "ui/canvas/Font.hpp"
#include "ui/canvas/Pen.hpp"

struct OverlayLook {

  MaskedIcon map_scale_left_icon;
  MaskedIcon map_scale_right_icon;

  const Font *overlay_font;

  Pen crosshair_pen;
  Pen crosshair_pen_alias;

  void Initialise(const Font &font, const Font &bold_font);
};
