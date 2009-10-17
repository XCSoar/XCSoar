#ifndef TASK_MACCREADY_HPP
#define TASK_MACCREADY_HPP

#include "Navigation/Aircraft.hpp"
#include "BaseTask/OrderedTaskPoint.hpp"
#include "GlideSolvers/MacCready.hpp"
#include <vector>

class TaskMacCready {
public:
  TaskMacCready(const std::vector<OrderedTaskPoint*> &_tps,
                const unsigned _activeTaskPoint,
                const double _mc):
    tps(_tps.begin(),_tps.end()),
    activeTaskPoint(_activeTaskPoint),
    start(0),
    end(_tps.size()-1),
    gs(_tps.size()),
    minHs(_tps.size(),0.0)
    {
      msolv.set_mc(_mc);
    };
  TaskMacCready(TaskPoint* tp,
                const double _mc):
    tps(1, tp),
    activeTaskPoint(0),
    start(0),
    end(0),
    gs(1),
    minHs(1,0.0)
    {
      msolv.set_mc(_mc);
    };

  GLIDE_RESULT glide_solution(const AIRCRAFT_STATE &aircraft);
  GLIDE_RESULT glide_sink(const AIRCRAFT_STATE &aircraft,
                          const double S);
  void print(std::ostream& f, const AIRCRAFT_STATE &aircraft) const;

  void set_mc(double mc) {
    msolv.set_mc(mc);
  };
  void set_cruise_efficiency(double ce) {
    msolv.set_cruise_efficiency(ce);
  };
  const GLIDE_RESULT& get_active_solution() {
    return gs[activeTaskPoint];
  };
protected:
  void clearance_heights(const AIRCRAFT_STATE &);
  virtual double get_min_height(const AIRCRAFT_STATE &aircraft) const = 0;
  virtual GLIDE_RESULT tp_solution(const unsigned i,
                                   const AIRCRAFT_STATE &aircraft, 
                                   double minH) const = 0;
  virtual const AIRCRAFT_STATE get_aircraft_start(const AIRCRAFT_STATE &aircraft) const = 0;

  const std::vector<TaskPoint*> tps;
  const unsigned activeTaskPoint;
  std::vector<double> minHs;
  int start;
  int end;
  std::vector<GLIDE_RESULT> gs;
  MacCready msolv;
  GLIDE_RESULT tp_sink(const unsigned i,
                       const AIRCRAFT_STATE &aircraft, 
                       const double S) const;
};

#endif
