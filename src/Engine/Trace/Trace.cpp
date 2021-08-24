/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "util/GlobalSliceAllocator.hxx"

#include <algorithm>
#include <iterator>

Trace::Trace(const Time _no_thin_time, const Time max_time,
             const unsigned max_size)
  :cached_size(0),
   max_time(max_time),
   no_thin_time(_no_thin_time),
   max_size(max_size),
   opt_size((3 * max_size) / 4)
{
  assert(max_size >= 4);
}

void
Trace::clear()
{
  assert(cached_size == delta_list.size());
  assert(cached_size == chronological_list.size());

  average_delta_distance = 0;
  average_delta_time = {};

  delta_list.clear();
  chronological_list.clear_and_dispose(MakeDisposer());
  cached_size = 0;

  assert(cached_size == delta_list.size());
  assert(cached_size == chronological_list.size());

  ++modify_serial;
  ++append_serial;
}

Trace::Time
Trace::GetRecentTime(const Time t) const noexcept
{
  if (empty())
    return {};

  const TracePoint &last = back();
  if (last.GetTime() > t)
    return last.GetTime() - t;

  return {};
}

void
Trace::UpdateDelta(TraceDelta &td)
{
  assert(cached_size == delta_list.size());
  assert(cached_size == chronological_list.size());

  if (&td == &chronological_list.front() ||
      &td == &chronological_list.back())
    return;

  const auto ci = chronological_list.iterator_to(td);
  const TraceDelta &previous = *std::prev(ci);
  const TraceDelta &next = *std::next(ci);

  delta_list.erase(delta_list.iterator_to(td));
  td.Update(previous.point, next.point);
  delta_list.insert(td);
}

void
Trace::EraseInside(DeltaList::iterator it)
{
  assert(cached_size > 0);
  assert(cached_size == delta_list.size());
  assert(cached_size == chronological_list.size());
  assert(it != delta_list.end());

  const TraceDelta &td = *it;
  assert(!td.IsEdge());

  const auto ci = chronological_list.iterator_to(td);
  TraceDelta &previous = const_cast<TraceDelta &>(*std::prev(ci));
  TraceDelta &next = const_cast<TraceDelta &>(*std::next(ci));

  // now delete the item
  chronological_list.erase(chronological_list.iterator_to(td));
  delta_list.erase_and_dispose(it, MakeDisposer());
  --cached_size;

  // and update the deltas
  UpdateDelta(previous);
  UpdateDelta(next);
}

bool
Trace::EraseDelta(const unsigned target_size, const Time recent) noexcept
{
  assert(cached_size == delta_list.size());
  assert(cached_size == chronological_list.size());

  if (size() <= 2)
    return false;

  bool modified = false;

  const Time recent_time = GetRecentTime(recent);

  auto candidate = delta_list.begin();
  while (size() > target_size) {
    const TraceDelta &td = *candidate;
    if (!td.IsEdge() && td.point.GetTime() < recent_time) {
      EraseInside(candidate);
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
Trace::EraseEarlierThan(const Time p_time) noexcept
{
  if (p_time == Time{} || empty() || GetFront().point.GetTime() >= p_time)
    // there will be nothing to remove
    return false;

  do {
    auto ci = chronological_list.begin();
    TraceDelta &td = *ci;
    chronological_list.erase(ci);

    auto i = delta_list.find(td);
    assert(i != delta_list.end());
    delta_list.erase_and_dispose(i, MakeDisposer());

    --cached_size;
  } while (!empty() && GetFront().point.GetTime() < p_time);

  // need to set deltas for first point, only one of these
  // will occur (have to search for this point)
  if (!empty())
    EraseStart(GetFront());

  ++modify_serial;
  ++append_serial;
  return true;
}

void
Trace::EraseLaterThan(const Time min_time) noexcept
{
  assert(min_time.count() > 0);
  assert(!empty());

  while (!empty() && GetBack().point.GetTime() > min_time) {
    TraceDelta &td = GetBack();

    chronological_list.erase(chronological_list.iterator_to(td));

    auto i = delta_list.find(td);
    assert(i != delta_list.end());
    delta_list.erase_and_dispose(i, MakeDisposer());

    --cached_size;
  }

  /* need to set deltas for first point, only one of these will occur
     (have to search for this point) */
  if (!empty())
    EraseStart(GetBack());
}

/**
 * Update start node (and neighbour) after min time pruning
 */
void
Trace::EraseStart(TraceDelta &td)
{
  delta_list.erase(delta_list.iterator_to(td));

  td.elim_distance = null_delta;
  td.elim_time = null_time;

  delta_list.insert(td);
}

void
Trace::push_back(const TracePoint &point)
{
  assert(cached_size == delta_list.size());
  assert(cached_size == chronological_list.size());

  const Time min_delta = std::chrono::seconds{2};

  if (empty()) {
    // first point determines origin for flat projection
    task_projection.Reset(point.GetLocation());
    task_projection.Update();
  } else if (point.GetTime() < back().GetTime()) {
    // gone back in time

    const Time clear_threshold = std::chrono::minutes{3};
    const Time fix_threshold = std::chrono::seconds{10};

    if (point.GetTime() + clear_threshold < back().GetTime()) {
      /* not fixable, clear the trace and restart from scratch */
      clear();
      return;
    }

    /* not much, try to fix it */
    EraseLaterThan(point.GetTime() - fix_threshold);
    ++modify_serial;
  } else if (point.GetTime() - back().GetTime() < min_delta)
    // only add one item per two seconds
    return;

  EnforceTimeWindow(point.GetTime());

  if (size() >= max_size)
    Thin();

  assert(size() < max_size);

  TraceDelta *td = allocator.allocate(1);
  std::allocator_traits<Allocator>::construct(allocator, td, point);
  td->point.Project(task_projection);

  delta_list.insert(*td);
  chronological_list.push_back(*td);

  ++cached_size;

  if (td != &chronological_list.front())
    UpdateDelta(*std::prev(chronological_list.iterator_to(*td)));

  ++append_serial;
}

unsigned
Trace::CalcAverageDeltaDistance(const Time no_thin) const noexcept
{
  const Time r = GetRecentTime(no_thin);
  unsigned acc = 0;
  unsigned counter = 0;

  const auto end = chronological_list.end();
  for (auto it = chronological_list.begin();
       it != end && it->point.GetTime() < r; ++it, ++counter)
    acc += it->delta_distance;

  if (counter)
    return acc / counter;

  return 0;
}

Trace::Time
Trace::CalcAverageDeltaTime(const Time no_thin) const noexcept
{
  const Time r = GetRecentTime(no_thin);
  unsigned counter = 0;

  /* find the last item before the "r" timestamp */
  const auto end = chronological_list.end();
  ChronologicalList::const_iterator it;
  for (it = chronological_list.begin(); it != end && it->point.GetTime() < r; ++it)
    ++counter;

  if (counter < 2)
    return {};

  --it;
  --counter;

  Time start_time = front().GetTime();
  Time end_time = it->point.GetTime();
  return (end_time - start_time) / counter;
}

void
Trace::EnforceTimeWindow(const Time latest_time) noexcept
{
  if (max_time == null_time)
    /* no time window configured */
    return;

  if (latest_time <= max_time)
    /* this can only happen if the flight launched shortly after
       midnight; this check is just here to avoid unsigned integer
       underflow */
    return;

  EraseEarlierThan(latest_time - max_time);
}

void
Trace::Thin2()
{
  const unsigned target_size = opt_size;
  assert(size() > target_size);

  // if still too big, remove points based on line simplification
  EraseDelta(target_size, no_thin_time);
  if (size() <= target_size)
    return;

  // if still too big, thin again, ignoring recency
  if (no_thin_time.count() > 0)
    EraseDelta(target_size, {});

  assert(size() <= target_size);
}

void
Trace::Thin()
{
  assert(cached_size == delta_list.size());
  assert(cached_size == chronological_list.size());
  assert(size() == max_size);

  Thin2();

  assert(size() < max_size);

  average_delta_distance = CalcAverageDeltaDistance(no_thin_time);
  average_delta_time = CalcAverageDeltaTime(no_thin_time);

  ++modify_serial;
  ++append_serial;
}

void
Trace::GetPoints(TracePointVector& iov) const
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
Trace::GetPoints(TracePointerVector &v) const
{
  v.clear();
  v.reserve(size());
  std::copy(PointerIterator<decltype(begin())>(begin()),
            PointerIterator<decltype(end())>(end()),
            std::back_inserter(v));
}

bool
Trace::SyncPoints(TracePointerVector &v) const
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
Trace::GetPoints(TracePointVector &v, const Time min_time,
                 const GeoPoint &location, double min_distance) const
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
