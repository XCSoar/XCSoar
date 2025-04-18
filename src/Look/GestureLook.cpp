// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GestureLook.hpp"
#include "Screen/Layout.hpp"

void
GestureLook::Initialise()
{
  pen.Create(Layout::ScalePenWidth(5), color);
  invalid_pen.Create(Layout::ScalePenWidth(5), invalid_color);
}

