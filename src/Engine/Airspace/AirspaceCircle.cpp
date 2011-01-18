/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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
#include "Navigation/Flat/FlatLine.hpp"
#include "Navigation/Flat/FlatBoundingBox.hpp"
#include "AirspaceIntersectSort.hpp"

AirspaceCircle::AirspaceCircle(const GeoPoint &loc, const fixed _radius):
  AbstractAirspace(CIRCLE),
  m_center(loc), 
  m_radius(_radius) {}

const FlatBoundingBox 
AirspaceCircle::get_bounding_box(const TaskProjection& task_projection) 
{
  static const Angle a225 = Angle::degrees(fixed(225));
  static const Angle a135 = Angle::degrees(fixed(135));
  static const Angle a045 = Angle::degrees(fixed(045));
  static const Angle a315 = Angle::degrees(fixed(315));

  const fixed eradius = m_radius * fixed(1.42);
  const GeoPoint ll = GeoVector(eradius, a225).end_point(m_center);
  const GeoPoint lr = GeoVector(eradius, a135).end_point(m_center);
  const GeoPoint ur = GeoVector(eradius, a045).end_point(m_center);
  const GeoPoint ul = GeoVector(eradius, a315).end_point(m_center);

  FlatGeoPoint fll = task_projection.project(ll);
  FlatGeoPoint flr = task_projection.project(lr);
  FlatGeoPoint ful = task_projection.project(ul);
  FlatGeoPoint fur = task_projection.project(ur);

  // note +/- 1 to ensure rounding keeps bb valid 

  return FlatBoundingBox(FlatGeoPoint(min(fll.Longitude, ful.Longitude) - 1,
                                      min(fll.Latitude, flr.Latitude) - 1),
                         FlatGeoPoint(max(flr.Longitude, fur.Longitude) + 1,
                                      max(ful.Latitude, fur.Latitude) + 1));
}

bool 
AirspaceCircle::inside(const GeoPoint &loc) const
{
  return (loc.distance(m_center) <= m_radius);
}

AirspaceIntersectionVector
AirspaceCircle::intersects(const GeoPoint& start, const GeoVector &vec) const
{
  const GeoPoint end = vec.end_point(start);
  AirspaceIntersectSort sorter(start, end, *this);

  const fixed f_radius = m_task_projection->fproject_range(m_center, m_radius);
  const FlatPoint f_center = m_task_projection->fproject(m_center);
  const FlatPoint f_start = m_task_projection->fproject(start);
  const FlatPoint f_end = m_task_projection->fproject(end);
  const FlatLine line(f_start, f_end);

  if (inside(start))
    sorter.add(fixed_zero, start);

  FlatPoint f_p1, f_p2;
  if (line.intersect_circle(f_radius, f_center, f_p1, f_p2)) {
    const fixed mag = line.mag_sq();
    if (positive(mag)) {
      fixed inv_mag = -fixed_one;
      const fixed t1 = FlatLine(f_start, f_p1).dot(line);
      const fixed t2 = (f_p1 == f_p2) ?
                       -fixed_one : FlatLine(f_start, f_p2).dot(line);

      const bool in_range = (t1 < mag) || (t2 < mag);
      // if at least one point is within range, capture both points

      if ((t1 >= fixed_zero) && in_range) {
        if (negative(inv_mag))
          inv_mag = fixed_one / mag;

        sorter.add(t1 * inv_mag, m_task_projection->funproject(f_p1));
      }

      if ((t2 >= fixed_zero) && in_range) {
        if (negative(inv_mag))
          inv_mag = fixed_one / mag;

        sorter.add(t2 * inv_mag, m_task_projection->funproject(f_p2));
      }
    }
  }

  return sorter.all();
}

GeoPoint 
AirspaceCircle::closest_point(const GeoPoint& loc) const
{
  const fixed d = loc.distance(m_center);
  if (d <= m_radius)
    return loc;
  else
    return m_center.intermediate_point(loc, m_radius);
}
