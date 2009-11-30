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
#ifndef FLATRAY_HPP
#define FLATRAY_HPP

#include "FlatGeoPoint.hpp"
#include "Math/fixed.hpp"

/**
 * Projected ray (a point and vector) in 2-d cartesian integer coordinates
 */
class FlatRay {
public:
/** 
 * Constructor given start/end locations
 * 
 * @param from Origin of ray
 * @param to End point of ray
 */
  FlatRay(const FLAT_GEOPOINT& from,
          const FLAT_GEOPOINT& to):
    point(from),vector(to-from),
    fx(vector.Longitude!=0? 1.0/vector.Longitude:0),
    fy(vector.Latitude!=0? 1.0/vector.Latitude:0) {};

  const FLAT_GEOPOINT point; /**< Origin of ray */
  const FLAT_GEOPOINT vector; /**< Vector representing ray direction and length */
  const fixed fx; /**< speedups for box intersection test */
  const fixed fy; /**< speedups for box intersection test */

/** 
 * Test whether two rays intersect
 * 
 * @param oray Other ray to test intersection with
 * 
 * @return Parameter [0,1] of vector on this ray that intersection occurs (or -1 if fail)
 */
  double intersects(const FlatRay &oray) const;

/** 
 * Parametric form of ray
 * 
 * @param t Parameter [0,1] of ray
 * 
 * @return Location of end point
 */
  FLAT_GEOPOINT parametric(const double t) const;
};

#endif
