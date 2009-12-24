/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Math/FastMath.h"
#include "Math/NavFunctions.hpp"
#include "Math/Geometry.hpp"
#include <algorithm>
#include "Math/fixed.hpp"

#define sqr(x) ((x)*(x))

FlatEllipse::FlatEllipse(const FlatPoint &_f1,
                         const FlatPoint &_f2,
                         const FlatPoint &_ap):
  f1(_f1),f2(_f2),ap(_ap)
{
  const FlatLine f12(f1,f2);
  p = f12.ave();
  theta = f12.angle();
  const fixed csq = f12.dsq();
  a = (f1.d(ap)+f2.d(ap));
  b = sqrt(a*a-csq)*fixed_half;
  a *= fixed_half;

  // a.sin(t)=ap.x
  // b.cos(t)=ap.y

  FlatPoint op = ap;
  op.sub(p);
  op.rotate(-theta);
  theta_initial = fixed_rad_to_deg*atan2(op.y*a, op.x*b);
}

fixed 
FlatEllipse::ab() const {
  return a/b;
}

fixed 
FlatEllipse::ba() const {
  return b/a;
}

FlatPoint 
FlatEllipse::parametric(const fixed t) const {
  const fixed at = (fixed_360*t+theta_initial)*fixed_deg_to_rad;
  fixed cat, sat;
  sin_cos(at,&sat,&cat);

  FlatPoint res(a*cat,b*sat);
  res.rotate(theta);
  res.add(p);
  return res;
}

bool 
FlatEllipse::intersect(const FlatLine &line, 
                       FlatPoint &i1, 
                       FlatPoint &i2) const 
{
  const fixed er = ab();
  const fixed ier = ba();
  FlatLine s_line = line;  
  
  s_line.sub(p);
  s_line.rotate(-theta);
  s_line.mul_y(er);
  
  if (s_line.intersect_czero(a, i1, i2)) {
    
    i1.mul_y(ier);
    i1.rotate(theta);
    i1.add(p);
    
    i2.mul_y(ier);
    i2.rotate(theta);
    i2.add(p);
    
    return true;
  } else {
    return false;
  }
}


bool FlatEllipse::intersect_extended(const FlatPoint &pe,
                                     FlatPoint &i1,
                                     FlatPoint &i2) const
{
  const FlatLine l_f1p(f1,pe);
  const FlatLine l_pf2(pe,f2);
  const fixed ang = l_f1p.angle();

  const fixed d = l_pf2.d()+max(a,b); // max line length

  fixed can,san;
  sin_cos(ang*fixed_deg_to_rad,&san,&can);

  FlatLine e_l(pe,FlatPoint(pe.x+d*can,
                            pe.y+d*san));
  // e_l is the line extended from p in direction of f1-p 
  
  return intersect(e_l, i1, i2);
}

// define an ellipse by three points,
// edge point, focus f1, focus f2

// r1+r2 = 2.a
// 

// e = sqrt(1.0-sqr(b/a)) = c/a
// .: c = a. sqrt(1.0-sqr(b/a))
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

