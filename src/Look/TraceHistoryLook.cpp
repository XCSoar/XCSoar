// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TraceHistoryLook.hpp"

void
TraceHistoryLook::Initialise(bool _inverse)
{
  inverse = _inverse;

  axis_pen.Create(1, COLOR_GRAY);
  line_pen.Create(2, inverse ? COLOR_WHITE : COLOR_BLACK);
}
