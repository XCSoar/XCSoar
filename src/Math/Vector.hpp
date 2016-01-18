/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef VECTOR_HPP
#define VECTOR_HPP

#include "Point2D.hpp"
#include "Geo/SpeedVector.hpp"

#include <math.h>

struct Vector : DoublePoint2D {
  Vector() = default;

  constexpr Vector(double _x, double _y):DoublePoint2D(_x, _y) {}

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

#endif
