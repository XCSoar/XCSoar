/* Copyright_License {

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

#ifndef FLATELLIPSE_HPP
#define FLATELLIPSE_HPP

#include "FlatPoint.hpp"
#include "Math/Angle.hpp"
#include "Compiler.h"

class FlatLine;

/**
 * 2-d ellipse in real-valued projected coordinates, with methods for
 * intersection tests etc.  The ellipse itself need not be axis-aligned.
 */
class FlatEllipse
{
  FlatPoint f1, f2, ap;
  FlatPoint p;
  double a;
  double b;
  Angle theta;

  Angle theta_initial;

public:
  /** 
   * Constructor.
   * 
   * @param _f1 Focus A
   * @param _f2 Focus B
   * @param _ap Any point on the ellipse
   * 
   * @return Initialised object
   */
  FlatEllipse(const FlatPoint &_f1, const FlatPoint &_f2, const FlatPoint &_ap);

  /**
   * Parametric representation of ellipse
   *
   * @param t Parameter [0,1]
   *
   * @return Location on ellipse
   */
  gcc_pure
  FlatPoint Parametric(double t) const;

  /**
   * Find intersection of line from focus 1 to p, through the ellipse
   *
   * @param p Reference point
   * @param i1 Intersection point 1 if found
   * @param i2 Intersection point 2 if found
   *
   * @return True if line intersects
   */
  bool IntersectExtended(const FlatPoint &p, FlatPoint &i1, FlatPoint &i2) const;

private:
  gcc_pure
  double ab() const {
    return a / b;
  }

  gcc_pure
  double ba() const {
    return b / a;
  }

  bool Intersect(const FlatLine &line, FlatPoint &i1, FlatPoint &i2) const;
};

#endif
