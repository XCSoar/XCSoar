// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"

class Font;

struct NextArrowLook
{
  Pen next_arrow_pen;
  Brush next_arrow_brush;

  const Font *font;

  void Initialise(const Font &font, bool use_colors = true, bool inverse = false);
};
