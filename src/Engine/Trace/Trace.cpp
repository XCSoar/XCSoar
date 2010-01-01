#include "Trace.hpp"
#include "Navigation/Aircraft.hpp"

Trace::Trace() :
  m_optimise_time(0)
{
}

void
Trace::append(const AIRCRAFT_STATE& state)
{
  if (empty()) {
    task_projection.reset(state.get_location());
    task_projection.update_fast();
    m_last_point.time = -1;
  } else if ((trace_tree.size() > 0) && (state.Time < m_last_point.time)) {
    clear();
    return;
  }

  TracePoint tp(state, task_projection);
  tp.last_time = m_last_point.time;
  trace_tree.insert(tp);
  m_last_point = tp;
}

void
Trace::optimise()
{
  trace_tree.optimize();
  m_optimise_time = m_last_point.time;
}

void
Trace::clear()
{
  trace_tree.clear();
  trace_tree.optimize();
  m_optimise_time = 0;
  m_last_point.time = -1;
}

unsigned
Trace::size() const
{
  return trace_tree.size();
}

bool
Trace::empty() const
{
  return trace_tree.empty();
}

Trace::TraceTree::const_iterator
Trace::begin() const
{
  return trace_tree.begin();
}

Trace::TraceTree::const_iterator
Trace::end() const
{
  return trace_tree.end();
}

struct time_out_of_range {
  time_out_of_range(const unsigned the_time = 0) :
    m_time(the_time)
  {
  }

  bool
  operator()(const TracePoint& x) const
  {
    return (x.time < m_time);
  }

  const unsigned m_time;
};

static bool
time_sort(const TracePoint& elem1, const TracePoint& elem2)
{
  return (elem1.time < elem2.time);
}

TracePointVector
Trace::find_within_range(const GEOPOINT &loc, const fixed range,
  const unsigned mintime) const
{
  AIRCRAFT_STATE state;
  state.Location = loc;
  TracePoint bb_target(state, task_projection);
  const unsigned mrange = task_projection.project_range(loc, range);

  TracePointVector vectors;
  trace_tree.find_within_range(bb_target, mrange, std::back_inserter(vectors));

  time_out_of_range time_pred(mintime);
  vectors.erase(std::remove_if(vectors.begin(), vectors.end(), time_pred),
      vectors.end());

  std::sort(vectors.begin(), vectors.end(), time_sort);

  return vectors;
}

bool
Trace::optimise_if_old()
{
  if (m_last_point.time > m_optimise_time + 60) {
    optimise();
    return true;
  } else {
    return false;
  }
}

static void
adjust_links(const TracePoint& previous, const TracePoint& obj,
    TracePoint& next)
{
  if ((obj.last_time == previous.time) && (next.last_time == obj.time)) {
    next.last_time = previous.time;
    next.Vario = (next.NavAltitude - previous.NavAltitude) / (next.time
        - previous.time);
    /// \todo work out scheme to merge NettoVario etc also
  }
}

void
Trace::thin_trace(TracePointVector& vec, const unsigned mrange_sq) const
{
  if (vec.size() < 2)
    return;

  for (TracePointVector::iterator it = vec.begin() + 1; it + 1 != vec.end();) {
    TracePointVector::iterator it_previous = it - 1;
    TracePointVector::iterator it_next = it + 1;

    if (it->approx_sq_dist(*it_previous) < mrange_sq) {
      adjust_links(*it_previous, *it, *it_next);
      vec.erase(it);
    } else {
      ++it;
    }
  }
}

void
Trace::thin_trace(TracePointVector& vec, const GEOPOINT &loc, const fixed range) const
{
  const unsigned mrange = task_projection.project_range(loc, range);

  thin_trace(vec, mrange * mrange);
}

TracePointVector
Trace::get_trace_points(unsigned max_points) const
{
  TracePointVector vectors;

  for (TraceTree::const_iterator it = begin(); it != end(); ++it) {
    vectors.push_back(*it);
  }

  if (vectors.empty())
    return vectors;

  std::sort(vectors.begin(), vectors.end(), time_sort);

  unsigned mrange = 3;

  do {
    thin_trace(vectors, mrange);
    mrange = (mrange * 4) / 3;
  } while (vectors.size() > max_points);

  // printf("trace points size %d mrange %d\n", vectors.size(), mrange);
  return vectors;
}
