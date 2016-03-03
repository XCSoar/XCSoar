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

#include "TestMath.hpp"
#include "Math/Quadrilateral.hpp"
#include "Math/Constants.hpp"
#include "TestUtil.hpp"

struct QuadrilateralInOut {
  DoublePoint2D g, l;
};

struct QuadrilateralTest {
  DoublePoint2D a, b, c, d;

  DoublePoint2D Map(DoublePoint2D p) const {
    return MapInQuadrilateral(a, b, c, d, p);
  }
};

static bool
Equals(DoublePoint2D a, DoublePoint2D b)
{
  return equals(a.x, b.x) && equals(a.y, b.y);
}

void
TestQuadrilateral()
{
  /* square */
  const QuadrilateralTest sq{{0,0}, {1,0}, {1,1}, {0,1}};
  ok1(Equals(sq.Map({0,0}), DoublePoint2D(0,0)));
  ok1(Equals(sq.Map({1,0}), DoublePoint2D(1,0)));
  ok1(Equals(sq.Map({1,1}), DoublePoint2D(1,1)));
  ok1(Equals(sq.Map({0,1}), DoublePoint2D(0,1)));

  /* rectangle 2x1 */
  const QuadrilateralTest rc21{{0,0}, {2,0}, {2,1}, {0,1}};
  ok1(Equals(rc21.Map({0,0}), DoublePoint2D(0,0)));
  ok1(Equals(rc21.Map({1,0}), DoublePoint2D(0.5,0)));
  ok1(Equals(rc21.Map({2,0}), DoublePoint2D(1,0)));
  ok1(Equals(rc21.Map({2,1}), DoublePoint2D(1,1)));
  ok1(Equals(rc21.Map({1,1}), DoublePoint2D(0.5,1)));
  ok1(Equals(rc21.Map({0,1}), DoublePoint2D(0,1)));

  /* rectangle 1x2 */
  const QuadrilateralTest rc12{{0,0}, {1,0}, {1,2}, {0,2}};
  ok1(Equals(rc12.Map({0,0}), DoublePoint2D(0,0)));
  ok1(Equals(rc12.Map({1,0}), DoublePoint2D(1,0)));
  ok1(Equals(rc12.Map({1,1}), DoublePoint2D(1,0.5)));
  ok1(Equals(rc12.Map({0,1}), DoublePoint2D(0,0.5)));
  ok1(Equals(rc12.Map({1,2}), DoublePoint2D(1,1)));
  ok1(Equals(rc12.Map({0,2}), DoublePoint2D(0,1)));

  /* trapezoid */
  const QuadrilateralTest trapezoid1{{0,0}, {1,0}, {2, 2}, {0,2}};
  ok1(Equals(trapezoid1.Map({0,0}), DoublePoint2D(0,0)));
  ok1(Equals(trapezoid1.Map({1,0}), DoublePoint2D(1,0)));
  ok1(Equals(trapezoid1.Map({2,2}), DoublePoint2D(1,1)));
  ok1(Equals(trapezoid1.Map({0,2}), DoublePoint2D(0,1)));
  ok1(Equals(trapezoid1.Map({0,1}), DoublePoint2D(0,0.5)));
  ok1(Equals(trapezoid1.Map({1.5,1}), DoublePoint2D(1,0.5)));
  ok1(Equals(trapezoid1.Map({0.5,1}), DoublePoint2D(1./3,0.5)));
  ok1(Equals(trapezoid1.Map({1,1}), DoublePoint2D(2./3,0.5)));

  const QuadrilateralTest trapezoid2{{0,1}, {2,0}, {2, 4}, {0,3}};
  ok1(Equals(trapezoid2.Map({0,1}), DoublePoint2D(0,0)));
  ok1(Equals(trapezoid2.Map({2,0}), DoublePoint2D(1,0)));
  ok1(Equals(trapezoid2.Map({2,4}), DoublePoint2D(1,1)));
  ok1(Equals(trapezoid2.Map({0,3}), DoublePoint2D(0,1)));
  ok1(Equals(trapezoid2.Map({0,2}), DoublePoint2D(0,0.5)));
  ok1(Equals(trapezoid2.Map({1,2}), DoublePoint2D(0.5,0.5)));
  ok1(Equals(trapezoid2.Map({2,2}), DoublePoint2D(1,0.5)));
  ok1(Equals(trapezoid2.Map({1,1}), DoublePoint2D(0.5,1./6)));
  ok1(Equals(trapezoid2.Map({1,0.5}), DoublePoint2D(0.5,0)));
  ok1(Equals(trapezoid2.Map({1,3.5}), DoublePoint2D(0.5,1)));

  /* square rotated by 90 degrees */
  const QuadrilateralTest sq90{{1,0}, {1,1}, {0,1}, {0,0}};
  ok1(Equals(sq90.Map({0,0}), DoublePoint2D(0,1)));
  ok1(Equals(sq90.Map({1,0}), DoublePoint2D(0,0)));
  ok1(Equals(sq90.Map({1,1}), DoublePoint2D(1,0)));
  ok1(Equals(sq90.Map({0,1}), DoublePoint2D(1,1)));

  /* square rotated by 180 degrees */
  const QuadrilateralTest sq180{{1,1}, {0,1}, {0,0}, {1,0}};
  ok1(Equals(sq180.Map({0,0}), DoublePoint2D(1,1)));
  ok1(Equals(sq180.Map({1,0}), DoublePoint2D(0,1)));
  ok1(Equals(sq180.Map({1,1}), DoublePoint2D(0,0)));
  ok1(Equals(sq180.Map({0,1}), DoublePoint2D(1,0)));

  /* square rotated by 270 degrees */
  const QuadrilateralTest sq270{{0,1}, {0,0}, {1,0}, {1,1}};
  ok1(Equals(sq270.Map({0,0}), DoublePoint2D(1,0)));
  ok1(Equals(sq270.Map({1,0}), DoublePoint2D(1,1)));
  ok1(Equals(sq270.Map({1,1}), DoublePoint2D(0,1)));
  ok1(Equals(sq270.Map({0,1}), DoublePoint2D(0,0)));

  /* square rotated by 45 degrees */
  const QuadrilateralTest sq45{{2,0}, {4,2}, {2,4}, {0,2}};
  ok1(Equals(sq45.Map({2,0}), DoublePoint2D(0,0)));
  ok1(Equals(sq45.Map({4,2}), DoublePoint2D(1,0)));
  ok1(Equals(sq45.Map({2,4}), DoublePoint2D(1,1)));
  ok1(Equals(sq45.Map({0,2}), DoublePoint2D(0,1)));
  ok1(Equals(sq45.Map({2,2}), DoublePoint2D(0.5,0.5)));
  ok1(Equals(sq45.Map({2,1}), DoublePoint2D(0.25,0.25)));

  /* try to produce rounding errors */
  const QuadrilateralTest r{{1e-50,0}, {M_PI,0}, {M_PI,M_PI}, {1e-50,M_PI}};
  ok1(Equals(r.Map({1e-50,0}), DoublePoint2D(0,0)));
  ok1(Equals(r.Map({M_PI,0}), DoublePoint2D(1,0)));
  ok1(Equals(r.Map({M_PI,M_PI}), DoublePoint2D(1,1)));
  ok1(Equals(r.Map({1e-50,M_PI}), DoublePoint2D(0,1)));
}
