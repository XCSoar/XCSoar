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
// http://mathworld.wolfram.com/Circle-FlatLineIntersection.html
// line two points (x1,y1), (x2,y2)
// Circle radius r at (0,0)

#include "FlatEllipse.hpp"
#include "FlatLine.hpp"
#include "Math/Util.hpp"

#include <algorithm>

#include <assert.h>

FlatEllipse::FlatEllipse(const FlatPoint &_f1, const FlatPoint &_f2,
                         const FlatPoint &_ap)
  :f1(_f1),f2(_f2),ap(_ap)
{
  const FlatLine f12(f1, f2);
  p = f12.GetMiddle();
  theta = f12.GetAngle();
  const auto csq = f12.GetSquaredDistance();
  a = (f1.Distance(ap) + f2.Distance(ap));

  assert(Square(a) >= csq);
  b = sqrt(Square(a) - csq) / 2;
  a /= 2;

  // a.sin(t) = ap.x
  // b.cos(t) = ap.y

  FlatPoint op = ap;
  op -= p;
  op.Rotate(-theta);
  theta_initial = Angle::FromXY(op.x * b, op.y * a).AsDelta();
}

FlatPoint
FlatEllipse::Parametric(const double t) const
{
  const Angle at = (Angle::FullCircle() * t + theta_initial).AsDelta();

  const auto sc = at.SinCos();
  double sat = sc.first, cat = sc.second;

  FlatPoint res(a * cat, b * sat);
  res.Rotate(theta);
  res += p;
  return res;
}

bool 
FlatEllipse::Intersect(const FlatLine &line, FlatPoint &i1, FlatPoint &i2) const
{
  const double er = ab();
  const double ier = ba();
  FlatLine s_line = line - p;

  s_line.Rotate(theta.Reciprocal());
  s_line.MultiplyY(er);

  if (s_line.IntersectOriginCircle(a, i1, i2)) {
    i1.MultiplyY(ier);
    i1.Rotate(theta);
    i1 += p;
    
    i2.MultiplyY(ier);
    i2.Rotate(theta);
    i2 += p;
    
    return true;
  }

  return false;
}

bool
FlatEllipse::IntersectExtended(const FlatPoint &pe, FlatPoint &i1,
                                FlatPoint &i2) const
{
  const FlatLine l_f1p(f1, pe);
  const FlatLine l_pf2(pe, f2);
  const Angle ang = l_f1p.GetAngle();

  const double d = l_pf2.GetDistance() + std::max(a, b); // max line length

  const auto sc = ang.SinCos();
  double san = sc.first, can = sc.second;

  FlatLine e_l(pe, FlatPoint(pe.x + d * can, pe.y + d * san));
  // e_l is the line extended from p in direction of f1-p 
  
  return Intersect(e_l, i1, i2);
}

// define an ellipse by three points,
// edge point, focus f1, focus f2

// r1+r2 = 2.a
// 

// e = sqrt(1.0-Square(b/a)) = c/a
// .: c = a. sqrt(1.0-Square(b/a))
//    c = sqrt(a*a-b*b)
//    c^2 = a*a-b*b

// .: 2.c = dist between f1 and f2

// given df12 = dist between f1 and f2
//        r1 = dist between f1 and p
//        r2 = dist between f2 and p
//
//    c = df12/2.0
//    a = (r1+r2)/2.0
//    b = sqrt(a*a-c*c)


// distances don't need to be shifted (a,b ok to calc)
// only line does

// w = (f2+f1)/2
// theta = atan2(f2-f1)

// shift line by -w
// rotate line by -theta
// solve intersection
// rotate result by theta
// shift by w

