#include "OnlineContest.hpp"
#include "Task/TaskEvents.hpp"
#include "Task/TaskBehaviour.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "PathSolvers/OLCDijkstra.hpp"

OnlineContest::OnlineContest(const TaskEvents &te, 
                             const TaskBehaviour &tb,
                             const GlidePolar &gp):
  m_task_events(te),
  m_task_behaviour(tb),
  m_glide_polar(gp)
{

}


bool 
OnlineContest::update_sample(const AIRCRAFT_STATE &state)
{
  bool do_add = false;

  if (m_sampled_points.empty()) {
    m_task_projection.reset(state.Location);
    m_task_projection.update_fast();
    do_add = true;
  } else {
    if (distance_is_significant(state.Location, m_sampled_points.back().get_location())) {
      do_add = true;
    }
  }
  if (!do_add) {
    return false;
  }

  SearchPoint sp(state.Location, m_task_projection, true);
  m_sampled_points.push_back(sp);

  return true;
}


bool 
OnlineContest::update_idle(const AIRCRAFT_STATE &state)
{
  OLCDijkstra dijkstra(*this, 3); // num stages, for testing now
  dijkstra.solve();

  return true;
}


void
OnlineContest::reset()
{
  m_sampled_points.clear();
}


const SearchPointVector& 
OnlineContest::get_sample_points() const
{
  return m_sampled_points;
}


void
OnlineContest::thin_samples()
{
  /// \todo thin samples if OLC samples gets too big
}


void 
OnlineContest::Accept(TaskPointVisitor& visitor, 
                      const bool reverse) const
{
  /// \todo - visit "OLCPoint"
}

bool 
OnlineContest::distance_is_significant(const GEOPOINT &location,
                                       const GEOPOINT &location_last) const
{
  SearchPoint a1(location, m_task_projection);
  SearchPoint a2(location_last, m_task_projection);
  return OLCDijkstra::distance_is_significant(a1, a2);
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
