// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TraceHistoryLook.hpp"
#include "Screen/Layout.hpp"

void
TraceHistoryLook::Initialise(bool _inverse)
{
  inverse = _inverse;

  axis_pen.Create(Layout::ScaleFinePenWidth(1), COLOR_GRAY);
  line_pen.Create(Layout::ScalePenWidth(2),
                  inverse ? COLOR_WHITE : COLOR_BLACK);
}
