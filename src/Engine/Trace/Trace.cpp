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
  if ((tp.time-m_last_point.time)<2) return;

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

class TracePointSetFilterInserter
{
public:
  TracePointSetFilterInserter(TracePointSet& the_set,
    const unsigned _min_time=0):
    m_set(&the_set),min_time(_min_time) {}

  TracePointSetFilterInserter& operator=(const TracePoint& val) {
    if (val.time>=min_time) {
      m_set->insert(val);
    }
    return *this;
  }
  TracePointSetFilterInserter& operator*() {
    return *this;
  }
  TracePointSetFilterInserter& operator++(const int x) {
    return *this;
  }
private:
  TracePointSet* m_set;
  unsigned min_time;
};

TracePointVector
Trace::find_within_range(const GEOPOINT &loc, const fixed range,
                         const unsigned mintime, const fixed resolution) const
{
  AIRCRAFT_STATE state; state.Location = loc; state.Time = mintime;
  TracePoint bb_target(state, task_projection);
  const unsigned mrange = task_projection.project_range(loc, range);
  const unsigned rrange = task_projection.project_range(loc, resolution);

//  TracePointVector vectors;
  TracePointSet tset;
  TracePointSetFilterInserter filter(tset, mintime);
  trace_tree.find_within_range(bb_target, mrange, filter);

/*
//                               std::inserter(tset, tset.begin()));
  if (mintime>0) {
    TracePointSet::iterator tit = tset.lower_bound(bb_target);
    tset.erase(tset.begin(), tit);
  }
*/
  if (positive(resolution)) {
    TracePointList tlist(tset.begin(), tset.end());
    thin_trace(tlist, rrange*rrange);
    return TracePointVector(tlist.begin(), tlist.end());
  } else {
    return TracePointVector(tset.begin(), tset.end());
  }
}

bool
Trace::optimise_if_old()
{
  if (m_last_point.time > m_optimise_time + 60) {
    optimise();
    return true;
  }

  return false;
}

static void
adjust_links(const TracePoint& previous, const TracePoint& obj,
    TracePoint& next)
{
  if ((obj.last_time == previous.time) && (next.last_time == obj.time)) {
    next.last_time = previous.time;
  }
}


void 
Trace::thin_trace(TracePointList& tlist, const unsigned mrange_sq) const
{
  if (tlist.size()<2) return;

  TracePointList::iterator it_prev = tlist.begin();
  TracePointList::iterator it = tlist.begin(); ++it;
  TracePointList::iterator it_next = it; ++it_next;

  for (; it_next != tlist.end(); ) {

    if (it->approx_sq_dist(*it_prev) < mrange_sq) {
      adjust_links(*it_prev, *it, *it_next);
      it = tlist.erase(it);
      it_next = it; ++it_next;
    } else {
      ++it;
      ++it_next;
      ++it_prev;
    }
  }
}


TracePointVector 
Trace::get_trace_points(const unsigned max_points) const
{
  TracePointSet tset(begin(),end());

  if (!tset.empty()) {

    TracePointList tlist(tset.begin(), tset.end());

    unsigned mrange = 3;
    while (tlist.size()>max_points) {
      thin_trace(tlist, mrange);
      mrange = (mrange*4)/3;
    }

    return TracePointVector(tlist.begin(), tlist.end());
  }

  return TracePointVector();
}
