#include "AirspaceIntersectSort.hpp"
#include "AbstractAirspace.hpp"

void 
AirspaceIntersectSort::add(const fixed t, const GEOPOINT &p)
{
  if ((t>=fixed_zero) && (t<fixed_one)) {
    m_q.push(std::make_pair(t,p));
  }
}

bool
AirspaceIntersectSort::empty() const
{
  return m_q.empty();
}

bool
AirspaceIntersectSort::top(GEOPOINT &p) const
{
  if (m_airspace->inside(m_start)) {
    p = m_start;
    return true;
  }
  if (!m_q.empty()) {
    p = m_q.top().second;
    return true;
  } else {
    return false;
  }
}

AirspaceIntersectionVector
AirspaceIntersectSort::all(const bool fill_end)
{
  AirspaceIntersectionVector res;

  GEOPOINT p_last = m_start;
  bool waiting = false;
  while (!m_q.empty()) {
    const GEOPOINT p_this = m_q.top().second; 
    const GEOPOINT p_mid = (p_this+p_last)*fixed_half;

    if (m_airspace->inside(p_mid)) {
      res.push_back(std::make_pair(p_last, p_this));
      waiting = false;
    } else {
      waiting = true;
    }

    // advance
    p_last = p_this;
    m_q.pop();
  }

  // fill last point (not really an intercept, but beyond range)
  if (waiting) {
    if (fill_end) {
      res.push_back(std::make_pair(p_last, m_end));
    } else {
      res.push_back(std::make_pair(p_last, p_last));
    }
  }
  return res;
}

