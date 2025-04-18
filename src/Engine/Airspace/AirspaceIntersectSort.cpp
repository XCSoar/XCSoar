// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceIntersectSort.hpp"
#include "AbstractAirspace.hpp"
#include "AirspaceIntersectionVector.hpp"

void
AirspaceIntersectSort::add(const double t, const GeoPoint &p) noexcept
{
  if (t >= 0)
    m_q.push(std::make_pair(t, p));
}

std::optional<GeoPoint>
AirspaceIntersectSort::top() const noexcept
{
  if (airspace.Inside(m_start))
    return m_start;

  if (!m_q.empty())
    return m_q.top().second;

  return std::nullopt;
}

AirspaceIntersectionVector
AirspaceIntersectSort::all() noexcept
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

    if (airspace.Inside(p_mid)) {
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
