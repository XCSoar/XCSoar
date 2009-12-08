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
#include "Navigation/GeoPoint.hpp"
#include "Math/Earth.hpp"

GEOPOINT 
GEOPOINT::parametric(const GEOPOINT &delta, const fixed t) const
{
  return (*this)+delta*t;
}

GEOPOINT 
GEOPOINT::interpolate(const GEOPOINT &end, const fixed t) const
{
  return (*this)+(end-(*this))*t;
}

fixed
GEOPOINT::distance(const GEOPOINT &other) const
{
  return ::Distance(*this, other);
}

fixed
GEOPOINT::bearing(const GEOPOINT &other) const
{
  return ::Bearing(*this, other);
}

fixed 
GEOPOINT::projected_distance(const GEOPOINT &from,
                             const GEOPOINT &to) const
{
  return ::ProjectedDistance(from, to, *this);
}

bool 
GEOPOINT::equals(const GEOPOINT &other) const
{
  return (Longitude == other.Longitude) && (Latitude == other.Latitude);
}

bool 
GEOPOINT::sort(const GEOPOINT &sp) const
{
  if (Longitude<sp.Longitude) {
    return false;
  } else if (Longitude==sp.Longitude) {
    return Latitude>sp.Latitude;
  } else {
    return true;
  }
}


GEOPOINT 
GEOPOINT::intermediate_point(const GEOPOINT &destination, 
                             const fixed distance) const
{
/* slow way */
  return ::IntermediatePoint(*this, destination, distance);
/* fast way (linear interpolation)
  GEOPOINT end = end_point(source);
  if (Distance>fixed_zero) {
    fixed t = distance/Distance;
    return source+(end-source)*t;
  } else {
    return source;
  }
*/
}
