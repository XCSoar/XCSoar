// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaveLook.hpp"
#include "Screen/Layout.hpp"

void
WaveLook::Initialise()
{
  pen.Create(Layout::ScalePenWidth(3), COLOR_BLUE);
}
