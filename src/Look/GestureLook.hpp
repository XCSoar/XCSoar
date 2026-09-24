// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Color.hpp"

struct GestureLook
{
  /** Colour of a recognised gesture */
  Color color;

  /** Colour of a gesture which is not (yet) recognised */
  Color invalid_color;

  /** Width of the line */
  unsigned width;

  void Initialise();
};
