/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Vector.hpp"
#include "Navigation/Aircraft.hpp"
#include "Util/GlobalSliceAllocator.hpp"

#include <algorithm>

Trace::Trace(const unsigned _no_thin_time, const unsigned max_time,
             const unsigned max_points)
  :chronological_list(ListHead::empty()),
   cached_size(0),
   m_max_time(max_time),
   no_thin_time(_no_thin_time),
   m_max_points(max_points),
   m_opt_points((3*max_points)/4)
{
  assert(max_points >= 4);
}

void
Trace::clear()
{
  assert(cached_size == delta_list.size());
  assert(cached_size == chronological_list.Count());

  m_average_delta_distance = 0;
  m_average_delta_time = 0;

  delta_list.clear();
  chronological_list.Clear();
  cached_size = 0;

  assert(cached_size == delta_list.size());
  assert(cached_size == chronological_list.Count());

  ++modify_serial;
}

unsigned
Trace::get_recent_time(const unsigned t) const
{
  if (empty())
    return 0;

  const TracePoint &last = back();
  if (last.GetTime() > t)
    return last.GetTime() - t;

  return 0;
}

void
Trace::update_delta(TraceDelta &td)
{
  assert(cached_size == delta_list.size());
  assert(cached_size == chronological_list.Count());

  if (chronological_list.IsEdge(td))
    return;

  const TraceDelta &previous = td.GetPrevious();
  const TraceDelta &next = td.GetNext();

  TraceDelta temp_td = td;
  temp_td.SetDisconnected();

  td.Replace(temp_td);

  // erase old one
  auto i = delta_list.find(td);
  assert(i != delta_list.end());
  delta_list.erase(i);

  // insert new in sorted position
  temp_td.update(previous.point, next.point);
  TraceDelta &new_td = insert(temp_td);
  new_td.SetDisconnected();
  temp_td.Replace(new_td);
}

void
Trace::erase_inside(TraceDelta::iterator it)
{
  assert(cached_size > 0);
  assert(cached_size == delta_list.size());
  assert(cached_size == chronological_list.Count());
  assert(it != delta_list.end());

  const TraceDelta &td = *it;
  assert(!td.IsEdge());

  TraceDelta &previous = const_cast<TraceDelta &>(td.GetPrevious());
  TraceDelta &next = const_cast<TraceDelta &>(td.GetNext());

  // now delete the item
  td.RemoveConst();
  delta_list.erase(it);
  --cached_size;

  // and update the deltas
  update_delta(previous);
  update_delta(next);
}

bool
Trace::erase_delta(const unsigned target_size, const unsigned recent)
{
  assert(cached_size == delta_list.size());
  assert(cached_size == chronological_list.Count());

  if (size() < 2)
    return false;

  bool modified = false;

  const unsigned recent_time = get_recent_time(recent);

  TraceDelta::iterator candidate = delta_list.begin();
  while (size() > target_size) {
    const TraceDelta &td = *candidate;
    if (!td.IsEdge() && td.point.GetTime() < recent_time) {
      erase_inside(candidate);
      candidate = delta_list.begin(); // find new top
      modified = true;
    } else {
      ++candidate;
      // suppressed removal, skip it.
    }
  }

  return modified;
}

bool
Trace::erase_earlier_than(const unsigned p_time)
{
  if (p_time == 0 || empty() || GetFront().point.GetTime() >= p_time)
    // there will be nothing to remove
    return false;

  do {
    TraceDelta &td = GetFront();
    td.Remove();

    auto i = delta_list.find(td);
    assert(i != delta_list.end());
    delta_list.erase(i);

    --cached_size;
  } while (!empty() && GetFront().point.GetTime() < p_time);

  // need to set deltas for first point, only one of these
  // will occur (have to search for this point)
  if (!empty())
    erase_start(GetFront());

  return true;
}

Trace::TraceDelta &
Trace::insert(const TraceDelta &td) {
  TraceDelta::iterator it = delta_list.insert(td);

  /* std::set doesn't allow modification of an item, but we
     override that */
  TraceDelta &new_td = const_cast<TraceDelta &>(*it);
  return new_td;
}

/**
 * Update start node (and neighbour) after min time pruning
 */
void
Trace::erase_start(TraceDelta &td_start) {
  TraceDelta temp_td = td_start;
  temp_td.SetDisconnected();
  td_start.Replace(temp_td);

  auto i_start = delta_list.find(td_start);
  assert(i_start != delta_list.end());
  delta_list.erase(i_start);

  temp_td.elim_distance = null_delta;
  temp_td.elim_time = null_time;

  TraceDelta &new_td = insert(temp_td);
  new_td.SetDisconnected();
  temp_td.Replace(new_td);
}

void
Trace::append(const AircraftState& state)
{
  assert(cached_size == delta_list.size());
  assert(cached_size == chronological_list.Count());

  if (empty()) {
    // first point determines origin for flat projection
    task_projection.reset(state.location);
    task_projection.update_fast();
  } else if (state.time < fixed(back().GetTime())) {
    // gone back in time, must reset. (shouldn't get here!)
    assert(1);
    clear();
    return;
  } else if ((unsigned)state.time - back().GetTime() < 2)
    // only add one item per two seconds
    return;

  if (size() >= m_max_points)
    Thin();

  assert(size() < m_max_points);

  TracePoint tp(state);
  tp.project(task_projection);

  TraceDelta &td = insert(tp);
  td.InsertBefore(chronological_list);

  ++cached_size;

  if (!chronological_list.IsFirst(td))
    update_delta(td.GetPrevious());

  ++append_serial;
}

unsigned
Trace::get_min_time() const
{
  if (empty() || m_max_time == null_time)
    return 0;

  unsigned last_time = back().GetTime();
  if (last_time == null_time || last_time <= m_max_time)
    return 0;

  return last_time - m_max_time;
}

unsigned
Trace::calc_average_delta_distance(const unsigned no_thin) const
{
  unsigned r = get_recent_time(no_thin);
  unsigned acc = 0;
  unsigned counter = 0;

  const ChronologicalConstIterator end = chronological_list.end();
  for (ChronologicalConstIterator it = chronological_list.begin();
       it != end && it->point.GetTime() < r; ++it, ++counter)
    acc += it->delta_distance;

  if (counter)
    return acc / counter;

  return 0;
}

unsigned
Trace::calc_average_delta_time(const unsigned no_thin) const
{
  unsigned r = get_recent_time(no_thin);
  unsigned counter = 0;

  /* find the last item before the "r" timestamp */
  const ChronologicalConstIterator end = chronological_list.end();
  ChronologicalConstIterator it;
  for (it = chronological_list.begin(); it != end && it->point.GetTime() < r; ++it)
    ++counter;

  if (counter < 2)
    return 0;

  --it;
  --counter;

  unsigned start_time = front().GetTime();
  unsigned end_time = it->point.GetTime();
  return (end_time - start_time) / counter;
}

void
Trace::Thin2()
{
  const unsigned target_size = m_opt_points;
  assert(size() > target_size);

  // first remove points outside max time range
  erase_earlier_than(get_min_time());
  if (size() <= target_size)
    return;

  // if still too big, remove points based on line simplification
  erase_delta(target_size, no_thin_time);
  if (size() <= target_size)
    return;

  // if still too big, thin again, ignoring recency
  if (no_thin_time > 0)
    erase_delta(target_size, 0);

  assert(size() <= target_size);
}

void
Trace::Thin()
{
  assert(cached_size == delta_list.size());
  assert(cached_size == chronological_list.Count());
  assert(size() == m_max_points);

  Thin2();

  assert(size() < m_max_points);

  m_average_delta_distance = calc_average_delta_distance(no_thin_time);
  m_average_delta_time = calc_average_delta_time(no_thin_time);

  ++modify_serial;
}

void
Trace::get_trace_points(TracePointVector& iov) const
{
  iov.clear();
  iov.reserve(size());
  std::copy(begin(), end(), std::back_inserter(iov));
}

template<typename I>
class PointerIterator {
  I i;

public:
  typedef typename I::iterator_category iterator_category;
  typedef typename I::pointer value_type;
  typedef typename I::pointer *pointer;
  typedef value_type &reference;
  typedef typename I::difference_type difference_type;

  PointerIterator() = default;
  explicit PointerIterator(I _i):i(_i) {}
  PointerIterator<I> &operator=(const PointerIterator<I> &other) = default;

  PointerIterator<I> &operator--() {
    --i;
    return *this;
  }

  PointerIterator<I> &operator++() {
    ++i;
    return *this;
  }

  typename I::pointer operator*() {
    return &*i;
  }

  bool operator==(const PointerIterator<I> &other) const {
    return i == other.i;
  }

  bool operator!=(const PointerIterator<I> &other) const {
    return i != other.i;
  }
};

void
Trace::GetTracePoints(TracePointerVector &v) const
{
  v.clear();
  v.reserve(size());
  std::copy(PointerIterator<decltype(begin())>(begin()),
            PointerIterator<decltype(end())>(end()),
            std::back_inserter(v));
}

bool
Trace::SyncTracePoints(TracePointerVector &v) const
{
  assert(v.size() <= size());

  if (v.size() == size())
    /* no news */
    return false;

  v.reserve(size());

  PointerIterator<decltype(end())> e(end());
  std::copy(std::prev(e, size() - v.size()), e,
            std::back_inserter(v));
  assert(v.size() == size());
  return true;
}

void
Trace::GetTracePoints(TracePointVector &v, unsigned min_time,
                      const GeoPoint &location, fixed min_distance) const
{
  /* skip the trace points that are before min_time */
  Trace::const_iterator i = begin(), end = this->end();
  unsigned skipped = 0;
  while (true) {
    if (i == end)
      /* nothing left */
      return;

    if (i->GetTime() >= min_time)
      /* found the first point that is within range */
      break;

    ++i;
    ++skipped;
  }

  assert(skipped < size());

  v.reserve(size() - skipped);
  const unsigned range = ProjectRange(location, min_distance);
  const unsigned sq_range = range * range;
  do {
    v.push_back(*i);
    i.NextSquareRange(sq_range, end);
  } while (i != end);
}
