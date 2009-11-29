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
#include "AirspacePolygon.hpp"
#include "Math/Earth.hpp"
#include "Navigation/ConvexHull/PolygonInterior.hpp"

AirspacePolygon::AirspacePolygon(const std::vector<GEOPOINT>& pts)
{
  TaskProjection task_projection;
  for (std::vector<GEOPOINT>::const_iterator v = pts.begin();
       v != pts.end(); v++) {
    border.push_back(SearchPoint(*v,task_projection));
  }

  /// \todo remove this pruning
  prune_interior(border);
}


const GEOPOINT 
AirspacePolygon::get_center()
{
  if (border.empty()) {
    return GEOPOINT(0,0);
  } else {
    return border[0].get_location();
  }
}


const FlatBoundingBox 
AirspacePolygon::get_bounding_box(const TaskProjection& task_projection)
{
  FLAT_GEOPOINT min;
  FLAT_GEOPOINT max;

  project(task_projection);

  bool empty=true;
  for (SearchPointVector::const_iterator v = border.begin();
       v != border.end(); v++) {
    FLAT_GEOPOINT f = v->get_flatLocation();
    if (empty) {
      empty = false;
      min = f; 
      max = f; 
    } else {
      min.Longitude = std::min(min.Longitude, f.Longitude);
      min.Latitude = std::min(min.Latitude, f.Latitude);
      max.Longitude = std::max(max.Longitude, f.Longitude);
      max.Latitude = std::max(max.Latitude, f.Latitude);
    }
  }
  if (!empty) {
    // note +/- 1 to ensure rounding keeps bb valid 
    min.Longitude-= 1; min.Latitude-= 1;
    max.Longitude+= 1; max.Latitude+= 1;
    return FlatBoundingBox(min,max);
  } else {
    return FlatBoundingBox(FLAT_GEOPOINT(0,0),FLAT_GEOPOINT(0,0));
  }
}

bool 
AirspacePolygon::inside(const AIRCRAFT_STATE &loc) const
{
  return PolygonInterior(loc.Location, border);
}


bool 
AirspacePolygon::intersects(const GEOPOINT& start, 
                            const GeoVector &vec,
                            const TaskProjection& task_projection) const
{
  if (PolygonInterior(start, border)) {
    // starts inside!
    return true;
  }

  const GEOPOINT end = vec.end_point(start);
  const FlatRay ray(task_projection.project(start),
                    task_projection.project(end));

  for (unsigned i=0; i+1<border.size(); i++) {
    const FlatRay rthis(border[i].get_flatLocation(), 
                        border[i+1].get_flatLocation());
    if (ray.intersects(rthis)) {
      return true;
    }
  }
  return false;
}

void
AirspacePolygon::project(const TaskProjection &task_projection)
{
  ::project(border, task_projection);
}
