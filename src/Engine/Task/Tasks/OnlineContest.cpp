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


static bool updated = false;

bool 
OnlineContest::update_sample(const AIRCRAFT_STATE &state)
{
  bool do_add = false;
  updated = false;

  if (m_trace_points.empty()) {
    m_task_projection.reset(state.Location);
    m_task_projection.update_fast();
    do_add = true;
  } else {

    if (distance_is_significant(state, m_trace_points.back())) {
      do_add = true;
      // \todo replace if lower even if not significant distance away
    }
  }
  if (!do_add) {
    return false;
  }

  TracePoint sp(state, m_task_projection);
  m_trace_points.push_back(sp);

  updated = true;

  return true;
}


void
OnlineContest::run_olc(OLCDijkstra &dijkstra)
{
  fixed score = dijkstra.score(common_stats.distance_olc);
  if (positive(score)) {
    dijkstra.copy_solution(m_solution);
  }
}


bool 
OnlineContest::update_idle(const AIRCRAFT_STATE &state)
{
  // \todo: possibly scan each type in a round robin fashion?

  if (updated) {

    switch (m_task_behaviour.olc_rules) {
    case OLC_Sprint:
      run_olc(olc_sprint);
      break;
    case OLC_FAI:
      run_olc(olc_fai);
      break;
    case OLC_Classic:
      run_olc(olc_classic);
      break;
    };
  }
  return true;
}


void
OnlineContest::reset()
{
  m_trace_points.clear();
  m_solution.clear();
}


const TracePointVector& 
OnlineContest::get_trace_points() const
{
  return m_trace_points;
}


void
OnlineContest::thin_trace()
{
  /// \todo thin trace if OLC samples gets too big
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

/*

- SearchPointVector find self intersections
  -- eliminate bad candidates
  -- remaining candidates are potential finish points

- OLCDijsktraRules
  -- max number of stages 
  -- distance weighting per stage
  -- retrieve point, num points per stage to search
  -- at stopping criterion (reached final stage),
     check extra rules (e.g. triangle leg lengths)
  -- (specialised for each type of OLC rule)

- OLCDijksktra
  (much like TaskDijsktra)

- Possible use of convex reduction for approximate solution to triangle

- Need to template SearchPointVector (and NavDijkstra) so it can have extras or less
  - GeoPoint
  - +FlatPoint
  - +Time
  - +Altitude

- Specialised thinning routine; store max/min altitude etc
*/
