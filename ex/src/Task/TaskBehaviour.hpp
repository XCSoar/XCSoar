#ifndef TASK_BEHAVIOUR_HPP
#define TASK_BEHAVIOUR_HPP

class TaskBehaviour 
{
public:

/** 
 * Constructor, sets default task behaviour
 * 
 */
  TaskBehaviour():
    optimise_targets_range(true),
    optimise_targets_bearing(true),
    auto_mc(true),
    calc_cruise_efficiency(true),
    aat_min_time(3600*4.9),
    safety_height_terrain(150.0),
    safety_height_finish(150.0),
    safety_height_arrival(300.0)
    {}

  bool optimise_targets_range;
  bool optimise_targets_bearing;
  bool auto_mc;
  bool calc_cruise_efficiency;
  double aat_min_time;
  double safety_height_terrain;
  double safety_height_finish;
  double safety_height_arrival;

/** 
 * Convenience function (used primarily for testing) to disable
 * all expensive task behaviour functions.
 */
  void all_off() {
    optimise_targets_range=false;
    optimise_targets_bearing=false;
    auto_mc=false;
    calc_cruise_efficiency=false;
  };

};

#endif
