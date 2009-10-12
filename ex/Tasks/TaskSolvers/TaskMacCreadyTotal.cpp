#include "TaskMacCreadyTotal.hpp"

TaskMacCreadyTotal::TaskMacCreadyTotal(const std::vector<OrderedTaskPoint*> &_tps,
                                       const unsigned _activeTaskPoint,
                                       const double _mc):
  TaskMacCready(_tps,_activeTaskPoint, _mc)
{
  start = 0;
  end = tps.size()-1;
}


GLIDE_RESULT 
TaskMacCreadyTotal::tp_solution(const unsigned i,
                                const AIRCRAFT_STATE &aircraft, 
                                double minH) const
{
  return tps[i]->glide_solution_planned(aircraft, msolv, minH);
}

const AIRCRAFT_STATE 
TaskMacCreadyTotal::get_aircraft_start(const AIRCRAFT_STATE &aircraft) const
{
  if (tps[0]->has_entered()) {
    return tps[0]->get_state_entered();
  } else {
    return aircraft;
  }
}

double 
TaskMacCreadyTotal::effective_distance(const double time_remaining) const
{
  // returns effective distance remaining such that at the virtual
  // point, the time remaining is the same as the reference time remaining 

  double t_total = 0.0;
  double d_total = 0.0;
  for (int i=end; i>=start; i--) {
    if (gs[i].TimeElapsed>0) {
      double p = (time_remaining-t_total)/gs[i].TimeElapsed;
      if ((p>=0.0) && (p<=1.0)) {
        return d_total+p*gs[i].Distance;
      }
      d_total += gs[i].Distance;
      t_total += gs[i].TimeElapsed;
    }
  }
  return d_total;
}

double 
TaskMacCreadyTotal::effective_leg_distance(const double time_remaining) const
{
  double p = (time_remaining)/gs[activeTaskPoint].TimeElapsed;
  return p*gs[activeTaskPoint].Distance;
}

