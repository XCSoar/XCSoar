// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Color.hpp"
#include "ui/canvas/Pen.hpp"

struct GestureLook
{
  static constexpr Color color = COLOR_RED;
  static constexpr Color invalid_color = LightColor(color);

  Pen pen, invalid_pen;

  void Initialise();
};
