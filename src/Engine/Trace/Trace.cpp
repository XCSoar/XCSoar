/*
Copyright_License {

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

#include "Trace.hpp"
#include "Navigation/Aircraft.hpp"
#include <algorithm>

Trace::Trace(const unsigned _no_thin_time, const unsigned max_time,
             const unsigned max_points)
  :m_max_time(max_time),
   no_thin_time(_no_thin_time),
   m_max_points(max_points),
   m_opt_points((3*max_points)/4)
{
  assert(max_points >= 4);
}

void
Trace::clear()
{
  chronological_list.clear();
  m_average_delta_distance = 0;
  m_average_delta_time = 0;
  delta_list.clear();
}

void
Trace::append(const AIRCRAFT_STATE& state)
{
  if (empty()) {
    // first point determines origin for flat projection
    task_projection.reset(state.get_location());
    task_projection.update_fast();
  } else if (state.Time < fixed(get_last_point().time)) {
    // gone back in time, must reset. (shouldn't get here!)
    assert(1);
    clear();
    return;
  } else if ((unsigned)state.Time - get_last_point().time < 2)
    // only add one item per two seconds
    return;

  TracePoint tp(state);
  tp.project(task_projection);

  chronological_list.push_back(tp);
  delta_list.append(chronological_list.back());

  assert(chronological_list.size() == delta_list.size());
}

unsigned
Trace::get_min_time() const
{
  if (empty() || m_max_time == null_time)
    return 0;

  unsigned last_time = get_last_point().time;
  if (last_time == null_time || last_time <= m_max_time)
    return 0;

  return last_time - m_max_time;
}

unsigned
Trace::calc_average_delta_distance(const unsigned no_thin) const
{
  unsigned r = delta_list.get_recent_time(no_thin);
  unsigned acc = 0;
  unsigned counter = 0;

  ChronologicalList::const_iterator end = chronological_list.end();
  for (ChronologicalList::const_iterator it = chronological_list.begin();
       it != end && it->point.time < r; ++it, ++counter)
    acc += it->delta_distance;

  if (counter)
    return acc / counter;

  return 0;
}

unsigned
Trace::calc_average_delta_time(const unsigned no_thin) const
{
  unsigned r = delta_list.get_recent_time(no_thin);
  unsigned counter = 0;

  /* find the last item before the "r" timestamp */
  ChronologicalList::const_iterator end = chronological_list.end();
  ChronologicalList::const_iterator it;
  for (it = chronological_list.begin(); it != end && it->point.time < r; ++it)
    ++counter;

  if (counter < 2)
    return 0;

  --it;
  --counter;

  unsigned start_time = chronological_list.front().point.time;
  unsigned end_time = it->point.time;
  return (end_time - start_time) / counter;
}

bool
Trace::optimise_if_old()
{
  if (chronological_list.size() >= m_max_points) {
    // first remove points outside max time range
    bool updated = delta_list.erase_earlier_than(get_min_time(),
                                                 chronological_list);

    if (chronological_list.size() >= m_opt_points)
      // if still too big, remove points based on line simplification
      updated |= delta_list.erase_delta(m_opt_points, chronological_list,
                                        no_thin_time);

    if (!updated)
      return false;

  } else if (chronological_list.size() * 2 == m_max_points) {
    // half size, appropriate time to remove old points
    if (!delta_list.erase_earlier_than(get_min_time(), chronological_list))
      return false;

  } else
    return false;

  m_average_delta_distance = calc_average_delta_distance(no_thin_time);
  m_average_delta_time = calc_average_delta_time(no_thin_time);

  return true;
}

unsigned
Trace::size() const
{
  return chronological_list.size();
}

bool
Trace::empty() const
{
  return delta_list.empty();
}

bool
Trace::is_null(const TracePoint& tp)
{
  return tp.time == null_time;
}

void
Trace::get_trace_points(TracePointVector& iov) const
{
  iov.clear();
  iov.reserve(size());
  std::copy(begin(), end(), std::back_inserter(iov));
}

void
Trace::get_trace_edges(TracePointVector &v) const
{
  v.clear();

  if (chronological_list.size() >= 2) {
    v.reserve(2);
    v.push_back(chronological_list.front().point);
    v.push_back(chronological_list.back().point);
  }
}
