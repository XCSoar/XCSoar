#include "OnlineContest.hpp"
#include "Task/TaskEvents.hpp"
#include "Task/TaskBehaviour.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Task/TaskStats/CommonStats.hpp"


OnlineContest::OnlineContest(const TaskEvents &te, 
                             const TaskBehaviour &tb,
                             const GlidePolar &gp,
                             CommonStats &stats):
  m_task_events(te),
  m_task_behaviour(tb),
  m_glide_polar(gp),
  common_stats(stats),
  olc_sprint(*this),
  olc_fai(*this),
  olc_classic(*this)
{
  reset();
}


bool 
OnlineContest::update_sample(const AIRCRAFT_STATE &state)
{
  bool do_add = false;

  if (m_trace_points.empty()) {
    m_task_projection.reset(state.Location);
    m_task_projection.update_fast();
    do_add = true;
  } else {

    if (distance_is_significant(state, m_trace_points.back())) {
      do_add = true;
    } else if (state.NavAltitude < m_trace_points.back().altitude) {
      // replace if lower even if not significant distance away
      m_trace_points.back().altitude = state.NavAltitude.as_int();
    }
  }
  if (!do_add) {
    return false;
  }

  TracePoint sp(state, m_task_projection);
  m_trace_points.push_back(sp);

  return true;
}


bool
OnlineContest::run_olc(OLCDijkstra &dijkstra)
{
  const fixed score = dijkstra.score(common_stats.distance_olc);
  if (positive(score)) {
    common_stats.time_olc = dijkstra.calc_time();
    if (positive(common_stats.time_olc)) {
      common_stats.speed_olc = dijkstra.calc_distance()/common_stats.time_olc;
    } else {
      common_stats.speed_olc = fixed_zero;
    }
    dijkstra.copy_solution(m_solution);
    return true;
  } else {
    return false;
  }
}


bool 
OnlineContest::update_idle(const AIRCRAFT_STATE &state)
{
  // \todo: possibly scan each type in a round robin fashion?
  bool retval = false;

  switch (m_task_behaviour.olc_rules) {
  case OLC_Sprint:
    retval = run_olc(olc_sprint);
    break;
  case OLC_FAI:
    retval = run_olc(olc_fai);
    break;
  case OLC_Classic:
    retval = run_olc(olc_classic);
    break;
  };

  if (retval) {
//    printf("time %d size %d\n", state.Time.as_int(), m_trace_points.size());
  }

  return retval;
}


void
OnlineContest::reset()
{
  m_trace_points.clear();
  m_solution.clear();
  olc_sprint.reset();
  olc_fai.reset();
  olc_classic.reset();
}


const TracePointVector& 
OnlineContest::get_trace_points() const
{
  return m_trace_points;
}


const TracePointVector& 
OnlineContest::get_olc_points() const
{
  return m_solution;
}


void 
OnlineContest::Accept(TaskPointVisitor& visitor, 
                      const bool reverse) const
{
  /// \todo - visit "OLCPoint"
}

bool 
OnlineContest::distance_is_significant(const AIRCRAFT_STATE &state,
                                       const TracePoint &state_last) const
{
  TracePoint a1(state, m_task_projection);
  return OLCDijkstra::distance_is_significant(a1, state_last, 10);
}


void 
OnlineContest::set_rank(const unsigned i, const unsigned d)
{
  m_trace_points[i].set_rank(d);
}

void 
OnlineContest::reset_rank()
{
  ::reset_rank(m_trace_points);
}

void
OnlineContest::prune()
{
#ifdef DO_PRINT
  print();
#endif
  if (m_trace_points.size()<300) {
    // no need to prune
    return;
  }
  /// \todo thin data based on rank

}

/*

- SearchPointVector find self intersections (for OLC-FAI)
  -- eliminate bad candidates
  -- remaining candidates are potential finish points

- Possible use of convex reduction for approximate solution to triangle

- Specialised thinning routine; store max/min altitude etc
*/
