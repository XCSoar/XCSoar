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
#include "Math/fixed.hpp"
#include "FlatBoundingBox.hpp"
#include "Math/FastMath.h"

unsigned 
FlatBoundingBox::distance(const FlatBoundingBox &f) const {
  long dx = max(0,min(f.bb_ll.Longitude-bb_ur.Longitude,
                      bb_ll.Longitude-f.bb_ur.Longitude));
  long dy = max(0,min(f.bb_ll.Latitude-bb_ur.Latitude,
                      bb_ll.Latitude-f.bb_ur.Latitude));
  return isqrt4(dx*dx+dy*dy);
}

void swap(fixed &t1, fixed &t2) 
{
  fixed t3 = t1;
  t1= t2;
  t2= t3;
}

bool 
FlatBoundingBox::intersects(const FlatRay& ray) const
{
  fixed tmin = fixed_zero;
  fixed tmax = fixed_one;
  
  // Longitude
  if (ray.vector.Longitude== fixed_zero) {
    // ray is parallel to slab. No hit if origin not within slab
    if ((ray.point.Longitude< bb_ll.Longitude) ||
        (ray.point.Longitude> bb_ur.Longitude)) {
      return false;
    }
  } else {
    // compute intersection t value of ray with near/far plane of slab
    fixed t1 = (bb_ll.Longitude-ray.point.Longitude)*ray.fx;
    fixed t2 = (bb_ur.Longitude-ray.point.Longitude)*ray.fx;
    // make t1 be intersection with near plane, t2 with far plane
    if (t1>t2) swap(t1, t2);
    tmin = max(tmin, t1);
    tmax = min(tmax, t2);
    // exit with no collision as soon as slab intersection becomes empty
    if (tmin>tmax) return false;
  }

  // Latitude
  // Longitude
  if (ray.vector.Latitude== fixed_zero) {
    // ray is parallel to slab. No hit if origin not within slab
    if ((ray.point.Latitude< bb_ll.Latitude) ||
        (ray.point.Latitude> bb_ur.Latitude)) {
      return false;
    }
  } else {
    // compute intersection t value of ray with near/far plane of slab
    fixed t1 = (bb_ll.Latitude-ray.point.Latitude)*ray.fy;
    fixed t2 = (bb_ur.Latitude-ray.point.Latitude)*ray.fy;
    // make t1 be intersection with near plane, t2 with far plane
    if (t1>t2) swap(t1, t2);
    tmin = max(tmin, t1);
    tmax = min(tmax, t2);
    // exit with no collision as soon as slab intersection becomes empty
    if (tmin>tmax) return false;
  }
  return true;
}

FLAT_GEOPOINT
FlatBoundingBox::get_center() const
{
  /// \todo This will break if overlaps 360/0
  FLAT_GEOPOINT c;
  c.Longitude = (bb_ll.Longitude+bb_ur.Longitude)/2;
  c.Latitude = (bb_ll.Latitude+bb_ur.Latitude)/2;
  return c;
}
