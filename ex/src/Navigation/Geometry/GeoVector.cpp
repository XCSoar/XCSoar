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
#include "GeoVector.hpp"
#include "Math/Earth.hpp"
#include "Math/NavFunctions.hpp"
#include "Math/Geometry.hpp"

GeoVector::GeoVector(const GEOPOINT &source, const GEOPOINT &target,
                     const bool is_average)
{
  GEOPOINT loc1 = source;
  GEOPOINT loc2 = target;
  ::DistanceBearing(loc1, loc2, &Distance, &Bearing);
}

GEOPOINT 
GeoVector::end_point(const GEOPOINT &source) const
{
  if (!positive(Distance)) {
    return source;
  } else {
    GEOPOINT p;
    ::FindLatitudeLongitude(source, Bearing, Distance, &p);
    return p;
  }
}

GEOPOINT 
GeoVector::mid_point(const GEOPOINT &source) const
{
  if (!positive(Distance)) {
    return source;
  } else {
    GEOPOINT p;
    ::FindLatitudeLongitude(source, Bearing, Distance*fixed_half, &p);
    return p;
  }
}

fixed
GeoVector::minimum_distance(const GEOPOINT &source,
                            const GEOPOINT &ref) const
{
  const GEOPOINT end = end_point(source);
  return (::CrossTrackError(source, end, ref, NULL));
}

GEOPOINT 
GeoVector::intermediate_point(const GEOPOINT &source, 
                              const fixed distance) const
{
  return source.intermediate_point(end_point(source), distance);
}


bool operator != (const GEOPOINT&g1, const GEOPOINT &g2) {
  return (g1.Latitude != g2.Latitude) || (g1.Longitude != g2.Longitude);
}

