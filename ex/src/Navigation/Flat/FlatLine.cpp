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
#include "FlatLine.hpp"
#include "Math/NavFunctions.hpp"

#define sqr(x) ((x)*(x))
#define sgn(x) (x<0? -1:1)

FlatPoint FlatLine::ave() const {
  FlatPoint p = p1;
  p.add(p2);
  p.x*= 0.5;
  p.y*= 0.5;
  return p;
}

double FlatLine::dx() const {
  return p2.x-p1.x;
}

double FlatLine::dy() const {
  return p2.y-p1.y;
}

double FlatLine::cross() const {
  return p1.cross(p2);
}

void FlatLine::mul_y(const double a) {
  p1.mul_y(a);
  p2.mul_y(a);
}

double FlatLine::d() const {
  return sqrt(dsq());
}

double FlatLine::dsq() const {
  const double _dx = dx();
  const double _dy = dy();
  return sqr(_dx)+sqr(_dy);
}

void FlatLine::sub(const FlatPoint&p) {
  p1.sub(p);
  p2.sub(p);
}

void 
FlatLine::add(const FlatPoint&p) {
  p1.add(p);
  p2.add(p);
}

double 
FlatLine::angle() const
{
  const double _dx = dx();
  const double _dy = dy();
  return RAD_TO_DEG*atan2(_dy,_dx);
}

void 
FlatLine::rotate(const double theta) 
{
  p1.rotate(theta);
  p2.rotate(theta);
}

bool 
FlatLine::intersect_czero(const double r,
                          FlatPoint &i1, FlatPoint &i2) const 
{
  const double _dx = dx();
  const double _dy = dy();
  const double dr = dsq();
  const double D = cross();
  
  double det = sqr(r)*dr-D*D;
  if (det<0) {
    // no solution
    return false;
  }
  det = sqrt(det);
  i1.x = (D*_dy+sgn(_dy)*_dx*det)/dr;
  i2.x = (D*_dy-sgn(_dy)*_dx*det)/dr;
  i1.y = (-D*_dx+fabs(_dy)*det)/dr;
  i2.y = (-D*_dx-fabs(_dy)*det)/dr;
  return true;
}
