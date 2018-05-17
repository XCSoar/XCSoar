/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef AIRSPACE_INTERSECT_SORT_HPP
#define AIRSPACE_INTERSECT_SORT_HPP

#include "Geo/GeoPoint.hpp"

#include <queue>

class AbstractAirspace;
class AirspaceIntersectionVector;

/**
 * Utility class to sort airspaces in ascending order of vector parameter (0,1)
 */
class AirspaceIntersectSort {
  typedef std::pair<double, GeoPoint> Intersection;

  /**
   * Function object used to rank intercepts by vector parameter t(0,1)
   */
  struct Rank : public std::binary_function<Intersection, Intersection, bool> {
    bool operator()(const Intersection& x, const Intersection& y) const {
      return x.first > y.first;
    }
  };

  std::priority_queue<Intersection, std::vector<Intersection>, Rank> m_q;

  const GeoPoint& m_start;
  const AbstractAirspace *m_airspace;

public:
  /**
   * Constructor
   *
   * @param start Location of start point
   * @param the_airspace Airspace to test for intersections
   */
  AirspaceIntersectSort(const GeoPoint &start,
                        const AbstractAirspace &the_airspace)
    :m_start(start), m_airspace(&the_airspace) {}

  /**
   * Add point to queue
   *
   * @param t Ray parameter [0,1]
   * @param p Point of intersection
   */
  void add(const double t, const GeoPoint &p);

  /**
   * Determine if no points are found
   *
   * @return True if no points added
   */
  bool empty() const {
    return m_q.empty();
  }

  /**
   * Return closest intercept point (or location if inside)
   *
   * @param p Point to set if any
   *
   * @return True if an intercept was found
   */
  bool top(GeoPoint &p) const;

  /**
   * Return vector of pairs of enter/exit intersections.
   *
   * @return vector
   */
  AirspaceIntersectionVector all();
};

#endif
