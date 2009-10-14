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
    tps(_tps),
    activeTaskPoint(_activeTaskPoint),
    start(0),
    end(tps.size()-1),
    gs(tps.size()),
    minHs(tps.size(),0.0)
    {
      msolv.set_mc(_mc);
    };

  GLIDE_RESULT glide_solution(const AIRCRAFT_STATE &aircraft);
  void clearance_heights(const AIRCRAFT_STATE &);
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
  virtual double get_min_height(const AIRCRAFT_STATE &aircraft) const = 0;
  virtual GLIDE_RESULT tp_solution(const unsigned i,
                                   const AIRCRAFT_STATE &aircraft, 
                                   double minH) const = 0;
  virtual const AIRCRAFT_STATE get_aircraft_start(const AIRCRAFT_STATE &aircraft) const = 0;

  const std::vector<OrderedTaskPoint*> &tps;
  const unsigned activeTaskPoint;
  std::vector<double> minHs;
  int start;
  int end;
  std::vector<GLIDE_RESULT> gs;
  MacCready msolv;
};

#endif
