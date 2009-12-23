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
#include "FlatPoint.hpp"
#include <algorithm>
#include <math.h>
#define sqr(x) ((x)*(x))

fixed 
FlatPoint::cross(const FlatPoint& p2) const {
  return x*p2.y-p2.x*y;
}

void 
FlatPoint::mul_y(const fixed a) {
  y*= a;
}
 
void 
FlatPoint::sub(const FlatPoint&p2) {
  x -= p2.x;
  y -= p2.y;
}

void 
FlatPoint::add(const FlatPoint&p2) {
  x += p2.x;
  y += p2.y;
}

void 
FlatPoint::rotate(const fixed angle) {
  const fixed _x = x;
  const fixed _y = y;
  fixed sa, ca;
  sin_cos(angle*fixed_deg_to_rad, &sa, &ca);
  x = _x*ca-_y*sa;
  y = _x*sa+_y*ca;
}

fixed 
FlatPoint::d(const FlatPoint &p) const {
  return sqrt(sqr(p.x-x)+sqr(p.y-y));
}

fixed
FlatPoint::mag_sq() const {
  return x*x+y*y;
}

fixed
FlatPoint::mag() const {
  return sqrt(mag_sq());
}
