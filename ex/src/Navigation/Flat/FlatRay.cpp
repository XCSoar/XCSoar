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
#include "FlatRay.hpp"

/**
 * Checks whether two lines 
 * intersect or not
 * @see http://local.wasp.uwa.edu.au/~pbourke/geometry/lineline2d/
 * adapted from line_line_intersection
 */
fixed
FlatRay::intersects (const FlatRay &oray) const
{
  const int denom = vector.cross(oray.vector);
  if (denom == 0) {
    // lines are parallel
    return -fixed_one;
  }
  const FLAT_GEOPOINT delta = point-oray.point;
  const int ua = vector.cross(delta);
  if (negative(ua) || (ua>denom)) {
    // outside first line
    return -fixed_one;
  } 
  const int ub = oray.vector.cross(delta);
  if (negative(ub) || (ub>denom)) {
    // outside second line
    return -fixed_one;
  }  

  // inside both lines
  return ((double)ua)/denom;
}


FLAT_GEOPOINT
FlatRay::parametric(const fixed t) const
{
  FLAT_GEOPOINT p = point;
  p.Longitude += vector.Longitude*FIXED_DOUBLE(t);
  p.Latitude += vector.Latitude*FIXED_DOUBLE(t);
  return p;
}
