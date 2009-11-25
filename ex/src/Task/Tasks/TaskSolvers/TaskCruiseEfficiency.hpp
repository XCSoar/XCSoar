#ifndef TASKCRUISEEFFICIENCY_HPP
#define TASKCRUISEEFFICIENCY_HPP

#include "TaskMacCreadyTravelled.hpp"
#include "Util/ZeroFinder.hpp"

/**
 *  Class to solve for cruise efficiency.
 *  This is the ratio of the achieved inter-thermal cruise speed to that
 *  predicted by MacCready theory with the current glide polar.
 *
 *  This is calculated for the part of the task that has been travelled
 */
class TaskCruiseEfficiency: 
  public ZeroFinder
{
public:
/** 
 * Constructor for ordered task points
 * 
 * @param tps Vector of ordered task points comprising the task
 * @param activeTaskPoint Current active task point in sequence
 * @param _aircraft Current aircraft state
 * @param gp Glide polar to copy for calculations
 */
  TaskCruiseEfficiency(const std::vector<OrderedTaskPoint*>& tps,
                       const unsigned activeTaskPoint,
                       const AIRCRAFT_STATE &_aircraft,
                       const GlidePolar &gp);
  virtual ~TaskCruiseEfficiency() {};

  virtual double f(const double ce);

/** 
 * Search for cruise efficiency value.
 * 
 * @param ce Default cruise efficiency value
 * 
 * @return Solution value of cruise efficiency
 */
  virtual double search(const double ce);

private:
  TaskMacCreadyTravelled tm;
  GlideResult res;
  const AIRCRAFT_STATE &aircraft;
  double dt;
};

#endif
