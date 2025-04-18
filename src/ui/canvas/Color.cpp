// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/Color.hpp"

Color
Desaturate(Color c) noexcept
{
#ifdef GREYSCALE
  return c;
#else
  int a = (c.Red() + c.Green() + c.Blue()) / 3;
  return Color((c.Red() + a) / 2, (c.Green() + a) / 2, (c.Blue() + a) / 2);
#endif
}
