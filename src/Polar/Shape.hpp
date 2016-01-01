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

#ifndef XCSOAR_POLAR_SHAPE_HPP
#define XCSOAR_POLAR_SHAPE_HPP

#include "Compiler.h"

#include <array>

struct PolarCoefficients;

struct PolarPoint {
  /**
   * Speed of point [m/s].
   */
  double v;

  /**
   * Sink rate of point [m/s].  Must be negative.
   */
  double w;
};

struct PolarShape {
  std::array<PolarPoint, 3> points;

  const PolarPoint &operator[](unsigned i) const {
    return points[i];
  }

  PolarPoint &operator[](unsigned i) {
    return points[i];
  }

  gcc_pure
  PolarCoefficients CalculateCoefficients() const;

  gcc_pure
  bool IsValid() const;
};

#endif
