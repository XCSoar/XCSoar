// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlatPoint.hpp"
#include "Math/Angle.hpp"

void
FlatPoint::Rotate(const Angle angle)
{
  const auto _x = x;
  const auto _y = y;
  const auto sc = angle.SinCos();
  const auto sa = sc.first, ca = sc.second;
  x = _x * ca - _y * sa;
  y = _x * sa + _y * ca;
}
