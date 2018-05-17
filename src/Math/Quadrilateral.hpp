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

#ifndef QUADRILATERAL_HPP
#define QUADRILATERAL_HPP

#include "Point2D.hpp"
#include "Compiler.h"

gcc_const
static double
CalcQuadrilateralU(const DoublePoint2D h, DoublePoint2D f,
                   DoublePoint2D e, DoublePoint2D g,
                   double v)
{
  return e.x + g.x * v == 0
          ? (h.y - f.y * v) / (e.y + g.y * v)
          : (h.x - f.x * v) / (e.x + g.x * v);
}

/**
 * Map a "global" point to a point inside the quadrilateral's frame of
 * reference.
 *
 * Formula from
 * http://www.iquilezles.org/www/articles/ibilinear/ibilinear.htm
 */
gcc_const
static DoublePoint2D
MapInQuadrilateral(const DoublePoint2D a, const DoublePoint2D b,
                   const DoublePoint2D c, const DoublePoint2D d,
                   const DoublePoint2D p)
{
  const DoublePoint2D e = b - a;
  const DoublePoint2D f = d - a;
  const DoublePoint2D g = (a - b) + (c - d);
  const DoublePoint2D h = p - a;

  const double k2 = CrossProduct(g, f);
  const double k1 = CrossProduct(e, f) + CrossProduct(h, g);
  const double k0 = CrossProduct(h, e);

  double w = k1 * k1 - 4.0 * k0 * k2;
  if (w < 0.0)
    return {-1, -1};

  if (k2 > -1e-10 && k2 < 1e-10) {
    const double v = -k0 / k1;
    const double u = CalcQuadrilateralU(h, f, e, g, v);
    return {u, v};
  }

  w = sqrt(w);

  const double v1 = (-k1 - w) / (2 * k2);
  const double u1 = CalcQuadrilateralU(h, f, e, g, v1);

  const double v2 = (-k1 + w) / (2 * k2);
  const double u2 = CalcQuadrilateralU(h, f, e, g, v2);

  double u = u1;
  double v = v1;

  if (v < 0.0 || v > 1.0 || u < 0.0 || u > 1.0 ) {
    u = u2;
    v = v2;
  }

  if (v < 0.0 || v > 1.0 || u < 0.0 || u > 1.0) {
    u = -1.0;
    v = -1.0;
  }

  return {u, v};
}

#endif
