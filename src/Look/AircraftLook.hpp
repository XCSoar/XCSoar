// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Pen.hpp"
#include "ui/canvas/Brush.hpp"

struct AircraftLook {
  Pen aircraft_pen;
  Pen aircraft_simple1_pen;
  Pen aircraft_simple2_pen;

  Pen canopy_pen;
  Brush canopy_brush;

  void Initialise();
};
