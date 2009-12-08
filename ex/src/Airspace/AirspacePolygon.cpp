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
#include "AirspaceIntersectSort.hpp"
#include <assert.h>

AirspacePolygon::AirspacePolygon(const std::vector<GEOPOINT>& pts,
  const bool prune)
{
  if (pts.size()<2) {
    m_is_convex = true;
  } else {
    TaskProjection task_projection; // note dummy blank projection
    for (std::vector<GEOPOINT>::const_iterator v = pts.begin();
         v != pts.end(); ++v) {
      m_border.push_back(SearchPoint(*v, task_projection));
    }
    // ensure airspace is closed
    GEOPOINT p_start = pts[0];
    GEOPOINT p_end = *(pts.end()-1);
    if (p_start != p_end) {
      m_border.push_back(SearchPoint(p_start, task_projection));
    }
    
    if (prune) {
      // only for testing
      prune_interior(m_border);
      m_is_convex = true;
    } else {
      m_is_convex = is_convex(m_border);
    }
  }
}


void
AirspacePolygon::project(const TaskProjection &task_projection)
{
  ::project(m_border, task_projection);
}


const GEOPOINT 
AirspacePolygon::get_center() const
{
  if (m_border.empty()) {
    return GEOPOINT(0,0);
  } else {
    return m_border[0].get_location();
  }
}


const FlatBoundingBox 
AirspacePolygon::get_bounding_box(const TaskProjection& task_projection)
{
  FLAT_GEOPOINT fmin;
  FLAT_GEOPOINT fmax;

  project(task_projection);

  bool empty=true;
  for (SearchPointVector::const_iterator v = m_border.begin();
       v != m_border.end(); ++v) {
    FLAT_GEOPOINT f = v->get_flatLocation();
    if (empty) {
      empty = false;
      fmin = f; 
      fmax = f; 
    } else {
      fmin.Longitude = min(fmin.Longitude, f.Longitude);
      fmin.Latitude = min(fmin.Latitude, f.Latitude);
      fmax.Longitude = max(fmax.Longitude, f.Longitude);
      fmax.Latitude = max(fmax.Latitude, f.Latitude);
    }
  }
  if (!empty) {
    // note +/- 1 to ensure rounding keeps bb valid 
    fmin.Longitude-= 1; fmin.Latitude-= 1;
    fmax.Longitude+= 1; fmax.Latitude+= 1;
    return FlatBoundingBox(fmin,fmax);
  } else {
    return FlatBoundingBox(FLAT_GEOPOINT(0,0),FLAT_GEOPOINT(0,0));
  }
}

bool 
AirspacePolygon::inside(const GEOPOINT &loc) const
{
  return PolygonInterior(loc, m_border);
}


AirspaceIntersectionVector
AirspacePolygon::intersects(const GEOPOINT& start, 
                            const GeoVector &vec,
                            const bool fill_end) const
{
  const GEOPOINT end = vec.end_point(start);
  const FlatRay ray(m_task_projection->project(start),
                    m_task_projection->project(end));

  AirspaceIntersectSort sorter(start, end, *this);

  for (SearchPointVector::const_iterator it= m_border.begin();
       it+1 != m_border.end(); ++it) {

    const FlatRay r_seg(it->get_flatLocation(), 
                        (it+1)->get_flatLocation());

    const fixed t = ray.intersects(r_seg);
    
    if ((t>=fixed_zero) && (t<fixed_one)) {
      sorter.add(t, m_task_projection->unproject(ray.parametric(t)));
    }
  }
  return sorter.all(fill_end);
}


GEOPOINT 
AirspacePolygon::closest_point(const GEOPOINT& loc) const
{
  const FLAT_GEOPOINT p = m_task_projection->project(loc);
  const FLAT_GEOPOINT pb = nearest_point(m_border, p, m_is_convex); 
  return m_task_projection->unproject(pb);
}
