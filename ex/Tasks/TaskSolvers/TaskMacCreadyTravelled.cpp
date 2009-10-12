#include "TaskMacCreadyTravelled.hpp"

TaskMacCreadyTravelled::TaskMacCreadyTravelled(const std::vector<OrderedTaskPoint*> &_tps,
                                               const unsigned _activeTaskPoint,
                                               const double _mc):
  TaskMacCready(_tps,_activeTaskPoint, _mc)
{
  start = 0;
  end = activeTaskPoint;
}


GLIDE_RESULT 
TaskMacCreadyTravelled::tp_solution(const unsigned i,
                                    const AIRCRAFT_STATE &aircraft, 
                                    double minH) const
{
  return tps[i]->glide_solution_travelled(aircraft, msolv, minH);
}

const AIRCRAFT_STATE 
TaskMacCreadyTravelled::get_aircraft_start(const AIRCRAFT_STATE &aircraft) const
{
  if (tps[0]->has_entered()) {
    return tps[0]->get_state_entered();
  } else {
    return aircraft;
  }
}


