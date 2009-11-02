
#include "TaskMacCreadyRemaining.hpp"

TaskMacCreadyRemaining::TaskMacCreadyRemaining(const std::vector<OrderedTaskPoint*> &_tps,
                                               const unsigned _activeTaskPoint,
                                               const GlidePolar _gp):
  TaskMacCready(_tps,_activeTaskPoint, _gp)
{
  start = activeTaskPoint;
  end = tps.size()-1;
}

TaskMacCreadyRemaining::TaskMacCreadyRemaining(TaskPoint* tp,
                                               const GlidePolar _gp):
  TaskMacCready(tp,_gp)
{
}

GLIDE_RESULT 
TaskMacCreadyRemaining::tp_solution(const unsigned i,
                                    const AIRCRAFT_STATE &aircraft, 
                                    double minH) const
{
  return tps[i]->glide_solution_remaining(aircraft, glide_polar, minH);
}


const AIRCRAFT_STATE 
TaskMacCreadyRemaining::get_aircraft_start(const AIRCRAFT_STATE &aircraft) const
{
  return aircraft;
}

void 
TaskMacCreadyRemaining::set_range(const double tp)
{
  for (int i=start; i<=end; i++) {
    tps[i]->set_range(tp);
  }
}
