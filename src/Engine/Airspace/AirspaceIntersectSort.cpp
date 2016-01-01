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

#include "AirspaceIntersectSort.hpp"
#include "AbstractAirspace.hpp"
#include "AirspaceIntersectionVector.hpp"

void 
AirspaceIntersectSort::add(const double t, const GeoPoint &p)
{
  if (t >= 0)
    m_q.push(std::make_pair(t, p));
}

bool
AirspaceIntersectSort::top(GeoPoint &p) const
{
  if (m_airspace->Inside(m_start)) {
    p = m_start;
    return true;
  }
  if (!m_q.empty()) {
    p = m_q.top().second;
    return true;
  }

  return false;
}

AirspaceIntersectionVector
AirspaceIntersectSort::all()
{
  AirspaceIntersectionVector res;

  GeoPoint p_last = m_start;
  bool waiting = false;
  bool start = true;
  while (!m_q.empty()) {
    const GeoPoint p_this = m_q.top().second;
    const GeoPoint p_mid = start
      ? p_last
      : p_last.Middle(p_this);

    // when inside, checking midpoint is ok, otherwise we should
    // check just beyond the last location

    if (m_airspace->Inside(p_mid)) {
      res.emplace_back(p_last, p_this);
      waiting = false;
    } else {
      if (m_q.top().first >= 1)
        // exit on reaching first point out of range
        break;

      waiting = true;
    }

    // advance
    p_last = p_this;
    m_q.pop();
    start = false;
  }

  // fill last point if not matched 
  if (waiting)
    res.emplace_back(p_last, p_last);

  return res;
}
