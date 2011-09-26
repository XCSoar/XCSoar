#include "AirspaceIntersectSort.hpp"
#include "AbstractAirspace.hpp"

void 
AirspaceIntersectSort::add(const fixed t, const GeoPoint &p)
{
  if (t >= fixed_zero)
    m_q.push(std::make_pair(t, p));
}

bool
AirspaceIntersectSort::empty() const
{
  return m_q.empty();
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
      : p_last.Interpolate(p_this, fixed_half);

    // when inside, checking midpoint is ok, otherwise we should
    // check just beyond the last location

    if (m_airspace->Inside(p_mid)) {
      res.push_back(std::make_pair(p_last, p_this));
      waiting = false;
    } else {
      if (m_q.top().first >= fixed_one)
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
    res.push_back(std::make_pair(p_last, p_last));

  return res;
}
