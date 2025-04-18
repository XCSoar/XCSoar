// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ClimbPercentLook.hpp"
#include "Screen/Layout.hpp"
#include "Resources.hpp"
#include "Colors.hpp"

void
ClimbPercentLook::Initialise()
{
  brush_circling_climb.Create(COLOR_GRAY);
  brush_noncircling_climb.Create(COLOR_GREEN);
  brush_circling_descent.Create(COLOR_ORANGE);
}
