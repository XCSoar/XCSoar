#ifndef TASK_MACCREADY_HPP
#define TASK_MACCREADY_HPP

#include "Navigation/Aircraft.hpp"
#include "Task/Tasks/BaseTask/OrderedTaskPoint.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "GlideSolvers/GlideResult.hpp"
#include <vector>

class TaskMacCready {
public:
  TaskMacCready(const std::vector<OrderedTaskPoint*> &_tps,
                const unsigned _activeTaskPoint,
                const GlidePolar &gp):
    tps(_tps.begin(),_tps.end()),
    activeTaskPoint(_activeTaskPoint),
    start(0),
    end(_tps.size()-1),
    gs(_tps.size()),
    minHs(_tps.size(),0.0),
    glide_polar(gp)
    {
    };
  TaskMacCready(TaskPoint* tp,
                const GlidePolar &gp):
    tps(1, tp),
    activeTaskPoint(0),
    start(0),
    end(0),
    gs(1),
    minHs(1,0.0),
    glide_polar(gp)
    {
    };

  GlideResult glide_solution(const AIRCRAFT_STATE &aircraft);
  GlideResult glide_sink(const AIRCRAFT_STATE &aircraft,
                          const double S);

  void set_mc(double mc) {
    glide_polar.set_mc(mc);
  };
  void set_cruise_efficiency(double ce) {
    glide_polar.set_cruise_efficiency(ce);
  };
  const GlideResult& get_active_solution() {
    return gs[activeTaskPoint];
  };
protected:
  void clearance_heights(const AIRCRAFT_STATE &);
  virtual double get_min_height(const AIRCRAFT_STATE &aircraft) const = 0;
  virtual GlideResult tp_solution(const unsigned i,
                                   const AIRCRAFT_STATE &aircraft, 
                                   double minH) const = 0;
  virtual const AIRCRAFT_STATE get_aircraft_start(const AIRCRAFT_STATE &aircraft) const = 0;

  const std::vector<TaskPoint*> tps;
  const unsigned activeTaskPoint;
  std::vector<double> minHs;
  int start;
  int end;
  std::vector<GlideResult> gs;
  GlidePolar glide_polar;
  GlideResult tp_sink(const unsigned i,
                       const AIRCRAFT_STATE &aircraft, 
                       const double S) const;
};

#endif
