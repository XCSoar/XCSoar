#include "Tasks/AbortTask.h"
#include "Navigation/Aircraft.hpp"
#include "BaseTask/TaskPoint.hpp"
#include <fstream>
#include <queue>

AbortTask::AbortTask(const TaskEvents &te, 
                     const TaskProjection &tp,
                     TaskAdvance &ta,
                     GlidePolar &gp,
                     const Waypoints &wps):
  AbstractTask(te, ta, gp), 
  task_projection(tp),
  waypoints(wps),
  polar_safety(gp),
  active_waypoint(-1)
{

}

AbortTask::~AbortTask()
{
  clear();
}

void AbortTask::setActiveTaskPoint(unsigned index)
{
  if (index<tps.size()) {
    activeTaskPoint = index;
    active_waypoint = tps[index].get_waypoint().id;
  }
}

TaskPoint* AbortTask::getActiveTaskPoint()
{
  if (activeTaskPoint<tps.size()) {
    return tps[activeTaskPoint];
  } else {
    return NULL;
  }
}


void AbortTask::report(const AIRCRAFT_STATE &state)
{
  std::ofstream f1("res-abort-task.txt");
  f1 << "#### Task points\n";
  for (unsigned i=0; i<tps.size(); i++) {
    GEOPOINT l = tps[i]->getLocation();
    f1 << "## point " << i << " ###################\n";
    f1 << state.Location.Longitude << " " << state.Location.Latitude << "\n";
    f1 << l.Longitude << " " << l.Latitude << "\n";
    f1 << "\n";
  }
}

typedef std::pair<WAYPOINT,double> WP_ALT;

struct Rank : public std::binary_function<WP_ALT, WP_ALT, bool> {
  bool operator()(const WP_ALT& x, const WP_ALT& y) const {
    return x.second > y.second;
  }
};

void AbortTask::clear() {
  for (std::vector< TaskPoint* >::iterator v=tps.begin();
       v != tps.end(); ) {
    delete (*v); 
    tps.erase(v);
  }
}


int 
AbortTask::abort_range(const AIRCRAFT_STATE &state)
{
  // always scan at least 50km or approx glide range
  double approx_range_m = 
    std::max(50000.0, state.Altitude*polar_safety.get_bestLD());

  int approx_range = task_projection.project_range(state.Location, approx_range_m);
  return approx_range;
}

void
AbortTask::update_polar()
{
  // glide_polar for task
  polar_safety = glide_polar;
  polar_safety.set_mc(0.0);
}

bool
AbortTask::task_full() const
{
  return (tps.size()>=10);
}

void
AbortTask::fill_reachable(const AIRCRAFT_STATE &state,
                          std::vector < WAYPOINT > &approx_waypoints,
                          const bool only_airfield)
{  
  if (task_full()) {
    return;
  }
  std::priority_queue<WP_ALT, std::vector<WP_ALT>, Rank> q;
  for (std::vector < WAYPOINT >::iterator v = approx_waypoints.begin();
       v!=approx_waypoints.end(); ) {
    TaskPoint t(*v);
    GLIDE_RESULT r = t.glide_solution_remaining(state, polar_safety);
    if (r.glide_reachable()) {
      q.push(std::make_pair(*v,r.TimeElapsed));
      // remove it since it's already in the list now      
      approx_waypoints.erase(v);
    } else {
      v++;
    }
  }
  while (!q.empty() && !task_full()) {
    tps.push_back(new TaskPoint(q.top().first));

    if (tps[tps.size()-1].get_waypoint().id == active_waypoint) {
      activeTaskPoint = i;
    }

    q.pop();
  }
}

bool AbortTask::update_sample(const AIRCRAFT_STATE &state, 
                              const bool full_update)
{
  update_polar();
  clear();

  activeTaskPoint = 0; // default to best result if can't find user-set one 

  std::vector < WAYPOINT > approx_waypoints = 
    waypoints.find_within_range_circle(state.Location, abort_range(state));

  if (!approx_waypoints.size()) {
    // TODO: increase range
  }

  // sort by alt difference

  // first try with safety polar
  fill_reachable(state, approx_waypoints, true);
  fill_reachable(state, approx_waypoints, false);

  // now try with non-safety polar
  polar_safety = glide_polar;
  fill_reachable(state, approx_waypoints, true);
  fill_reachable(state, approx_waypoints, false);

  // now try with fake height added
  AIRCRAFT_STATE fake = state;
  fake.Altitude += 10000.0;
  fill_reachable(state, approx_waypoints, true);
  fill_reachable(state, approx_waypoints, false);

  // TODO, check tracking of active waypoint

  if (tps.size()) {
    active_waypoint = tps[activeTaskPoint].get_waypoint().id;
  }

  return false; // nothing to do
}


bool 
AbortTask::check_transitions(const AIRCRAFT_STATE &, const AIRCRAFT_STATE&)
{
  return false; // nothing to do
}

