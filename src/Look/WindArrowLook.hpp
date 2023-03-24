// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"

class Font;

struct WindArrowLook
{
  Pen arrow_pen, shaft_pen;
  Brush arrow_brush;
  Brush arrow_brush_extern;
  Brush arrow_brush_instantaneous;

  const Font *font;

  void Initialise(const Font &font, bool inverse = false);
};
