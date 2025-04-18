// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Point2D.hpp"
#include "Geo/SpeedVector.hpp"

#include <math.h>

struct Vector : DoublePoint2D {
  Vector() = default;
  using DoublePoint2D::DoublePoint2D;

  Vector(Angle bearing, double norm) {
    auto sc = bearing.SinCos();
    x = sc.second * norm;
    y = sc.first * norm;
  }

  Vector(const SpeedVector speed) {
    auto sc = speed.bearing.SinCos();
    x = sc.second * speed.norm;
    y = sc.first * speed.norm;
  }
};
