#ifndef ABSTRACTTASK_H
#define ABSTRACTTASK_H

#include "TaskInterface.h"
#include "TaskEvents.hpp"
#include "TaskBehaviour.hpp"
#include "TaskAdvance.hpp"
#include "TaskStats/TaskStats.hpp"
#include "GlideSolvers/GlidePolar.hpp"

class AbstractTask : public TaskInterface {
public:
  AbstractTask(const TaskEvents &te,
               const TaskBehaviour &tb,
               TaskAdvance &ta,
               GlidePolar &gp): 
    activeTaskPoint(0),
    task_events(te),
    task_advance(ta),
    task_behaviour(tb),
    glide_polar(gp)
  {};

    unsigned getActiveTaskPointIndex();

    virtual const TaskStats& get_stats() const {
      return stats;
    }
    bool update(const AIRCRAFT_STATE &, const AIRCRAFT_STATE&);
    
    virtual bool update_idle(const AIRCRAFT_STATE&);
    
  virtual void report(const AIRCRAFT_STATE&);

protected:
  virtual bool update_sample(const AIRCRAFT_STATE &, 
                             const bool full_update) = 0;
  virtual bool check_transitions(const AIRCRAFT_STATE &, 
                                 const AIRCRAFT_STATE&) = 0;
  
  unsigned activeTaskPoint;
  TaskStats stats;

  virtual double calc_mc_best(const AIRCRAFT_STATE &);

  virtual double calc_glide_required(const AIRCRAFT_STATE &aircraft);

  virtual double calc_cruise_efficiency(const AIRCRAFT_STATE &aircraft) {
    return 1.0;
  }
  virtual double calc_min_target(const AIRCRAFT_STATE &, 
                                 const double t_target) {
    return 0.0;
  };

  virtual double scan_total_start_time(const AIRCRAFT_STATE &);
  virtual double scan_leg_start_time(const AIRCRAFT_STATE &);

  virtual void scan_distance_minmax(const GEOPOINT &location, bool full,
                                    double *dmin, double *dmax);
  virtual double scan_distance_nominal();
  virtual double scan_distance_planned();
  virtual double scan_distance_scored(const GEOPOINT &location);
  virtual double scan_distance_travelled(const GEOPOINT &location);
  virtual double scan_distance_remaining(const GEOPOINT &location);

  virtual void glide_solution_remaining(const AIRCRAFT_STATE &, 
                                        GLIDE_RESULT &total,
                                        GLIDE_RESULT &leg);
  virtual void glide_solution_travelled(const AIRCRAFT_STATE &, 
                                        GLIDE_RESULT &total,
                                        GLIDE_RESULT &leg);
  virtual void glide_solution_planned(const AIRCRAFT_STATE &, 
                                      GLIDE_RESULT &total,
                                      GLIDE_RESULT &leg,
                                      DistanceRemainingStat &total_remaining_effective,
                                      DistanceRemainingStat &leg_remaining_effective,
                                      const double total_t_elapsed,
                                      const double leg_t_elapsed);

  virtual double calc_gradient(const AIRCRAFT_STATE &state);

protected:
  const TaskEvents &task_events;
  TaskAdvance &task_advance;
  GlidePolar glide_polar;
  const TaskBehaviour &task_behaviour;

private:
  void update_glide_solutions(const AIRCRAFT_STATE &state);
  void update_stats_distances(const GEOPOINT &location,
                              const bool full_update);
  void update_stats_times(const AIRCRAFT_STATE &);
  void update_stats_speeds(const AIRCRAFT_STATE &, const AIRCRAFT_STATE&);
  void update_stats_glide(const AIRCRAFT_STATE &state);
  double leg_gradient(const AIRCRAFT_STATE &state);
};
#endif //ABSTRACTTASK_H
