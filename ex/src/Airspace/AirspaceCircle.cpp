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
#include "AirspaceCircle.hpp"
#include "Navigation/Geometry/GeoVector.hpp"
#include "Math/Earth.hpp"

AirspaceCircle::AirspaceCircle(const GEOPOINT &loc, 
                               const double _radius):
  center(loc), 
  radius(_radius)
{

}

const GEOPOINT 
AirspaceCircle::get_center()
{
  return center;
}


const FlatBoundingBox 
AirspaceCircle::get_bounding_box(const TaskProjection& task_projection) 
{
  const double eradius = radius*1.42;
  const GEOPOINT ll = GeoVector(eradius,225).end_point(center);
  const GEOPOINT lr = GeoVector(eradius,135).end_point(center);
  const GEOPOINT ur = GeoVector(eradius,45).end_point(center);
  const GEOPOINT ul = GeoVector(eradius,315).end_point(center);

  FLAT_GEOPOINT fll = task_projection.project(ll);
  FLAT_GEOPOINT flr = task_projection.project(lr);
  FLAT_GEOPOINT ful = task_projection.project(ul);
  FLAT_GEOPOINT fur = task_projection.project(ur);

  // note +/- 1 to ensure rounding keeps bb valid 

  return FlatBoundingBox(FLAT_GEOPOINT(std::min(fll.Longitude,
                                                ful.Longitude)-1, 
                                       std::min(fll.Latitude,
                                                flr.Latitude)-1), 
                         FLAT_GEOPOINT(std::max(flr.Longitude,
                                                fur.Longitude)+1, 
                                       std::max(ful.Latitude,
                                                fur.Latitude)+1));
}

bool 
AirspaceCircle::inside(const AIRCRAFT_STATE &loc) const
{
  return (loc.Location.distance(center)<=radius);
}

bool 
AirspaceCircle::intersects(const GEOPOINT& start, const GeoVector &vec,
                           const TaskProjection& task_projection) const
{
  const GEOPOINT end = vec.end_point(start);
  return (::CrossTrackError(start, end, center, NULL)<=radius);
}
