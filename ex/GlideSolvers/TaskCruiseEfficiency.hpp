#ifndef TASKCRUISEEFFICIENCY_HPP
#define TASKCRUISEEFFICIENCY_HPP

#include "TaskMacCreadyTravelled.hpp"
#include "ZeroFinder.hpp"

class TaskCruiseEfficiency: 
  public ZeroFinder
{
public:
  TaskCruiseEfficiency(const std::vector<OrderedTaskPoint*>& tps,
                       const unsigned activeTaskPoint,
                       const AIRCRAFT_STATE &_aircraft,
                       const double mc):
    ZeroFinder(0.1,2.0,0.01),
    tm(tps,activeTaskPoint,mc),
    aircraft(_aircraft) 
    {
      dt = aircraft.Time-tps[0]->get_state_entered().Time;
    };
  virtual double f(double ce) {
    tm.set_cruise_efficiency(ce);
    res = tm.glide_solution(aircraft);
    double d = fabs(res.TimeElapsed-dt);
    if (!res.Solution==MacCready::RESULT_OK) {
      d += res.TimeVirtual;
    }
    return d;
  }
  virtual bool valid(double ce) {
    tm.set_cruise_efficiency(ce);
    res = tm.glide_solution(aircraft);
    return (res.Solution== MacCready::RESULT_OK);
  }
  virtual double search(double ce) {
    return find_min(ce);
  }
protected:
  TaskMacCreadyTravelled tm;
  GLIDE_RESULT res;
  const AIRCRAFT_STATE &aircraft;
  double dt;
};




#endif
