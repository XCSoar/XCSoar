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
#include <algorithm>

unsigned count_distbearing = 0;

GeoVector::GeoVector(const GEOPOINT &source, const GEOPOINT &target,
                     const bool is_average)
{

  count_distbearing++;

  GEOPOINT loc1 = source;
  GEOPOINT loc2 = target;

  loc1.Latitude *= DEG_TO_RAD;
  loc2.Latitude *= DEG_TO_RAD;
  loc1.Longitude *= DEG_TO_RAD;
  loc2.Longitude *= DEG_TO_RAD;

  const double cloc1Latitude = cos(loc1.Latitude);
  const double cloc2Latitude = cos(loc2.Latitude);
  const double dlon = loc2.Longitude-loc1.Longitude;

  const double s1 = sin((loc2.Latitude-loc1.Latitude)/2);
  const double s2 = sin(dlon/2);
  const double a= std::max(0.0,std::min(1.0,s1*s1+cloc1Latitude*cloc2Latitude*s2*s2));
  Distance = 6371000.0*2.0*atan2(sqrt(a),sqrt(1.0-a));

  const double y = sin(dlon)*cloc2Latitude;
  const double x = cloc1Latitude*sin(loc2.Latitude)
    -sin(loc1.Latitude)*cloc2Latitude*cos(dlon);

  Bearing = (x==0 && y==0) ? 0:AngleLimit360(atan2(y,x)*RAD_TO_DEG);

}

GEOPOINT 
GeoVector::end_point(const GEOPOINT &source) const
{
  GEOPOINT p;
  ::FindLatitudeLongitude(source, Bearing, Distance, &p);
  return p;
}

GEOPOINT 
GeoVector::mid_point(const GEOPOINT &source) const
{
  GEOPOINT p;
  ::FindLatitudeLongitude(source, Bearing, Distance/2.0, &p);
  return p;
}

bool operator != (const GEOPOINT&g1, const GEOPOINT &g2) {
  return (g1.Latitude != g2.Latitude) || (g1.Longitude != g2.Longitude);
}

/* unused
bool operator != (const GeoVector&g1, const GeoVector &g2) {
  return (g1.Distance != g2.Distance) || (g1.Bearing != g2.Bearing);
}
*/

