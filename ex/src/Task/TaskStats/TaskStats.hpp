#ifndef TASKSTATS_HPP
#define TASKSTATS_HPP

#include "GlideSolvers/GlideResult.hpp"
#ifdef DO_PRINT
#include <iostream>
#endif
#include "Util/Filter.hpp"

struct AIRCRAFT_STATE;

class ElementStat;

class DistanceStat
{
public:
/** 
 * Constructor; initialises all to zero
 * 
 */
  DistanceStat();

/** 
 * Setter for distance value
 * 
 * @param d Distance value (m)
 */
  void set_distance(const double d) {
    distance = d;
  }

/** 
 * Accessor for distance value
 * 
 * @return Distance value (m)
 */
  double get_distance() const {
    return distance;
  };

/** 
 * Accessor for speed
 * 
 * @return Speed (m/s)
 */  double get_speed() const {
    return speed;
  };

/** 
 * Accessor for incremental speed (rate of change of
 * distance over dt, low-pass filtered)
 * 
 * @return Speed incremental (m/s)
 */
  double get_speed_incremental() const {
    return speed_incremental;
  };

/** 
 * Calculate bulk speed (distance/time), abstract base method
 * 
 * @param es ElementStat (used for time access)
 */
  virtual void calc_speed(const ElementStat* es) = 0;

/** 
 * Calculate incremental speed from previous step.
 * Resets incremental speed to speed if dt=0
 * 
 * @param dt Time step (s)
 */
  virtual void calc_incremental_speed(const double dt);

  friend std::ostream& operator<< (std::ostream& o, 
                                   const DistanceStat& ds);

protected:
  double distance;
  double distance_last;
  double speed;
  double speed_incremental;
  int counter;
  Filter lpf;
};

class DistanceRemainingStat:
  public DistanceStat
{
public:
/** 
 * Calculate speed (distance remaining/time remaining)
 * 
 * @param es ElementStat (used for time access)
 */
  virtual void calc_speed(const ElementStat* es);
};

class DistancePlannedStat:
  public DistanceStat
{
public:
/** 
 * Calculate speed (distance planned/time planned)
 * 
 * @param es ElementStat (used for time access)
 */
  virtual void calc_speed(const ElementStat* es);
};

class DistanceTravelledStat:
  public DistanceStat
{
public:
/** 
 * Calculate speed (distance travelled/time elapsed)
 * 
 * @param es ElementStat (used for time access)
 */
  virtual void calc_speed(const ElementStat* es);

/** 
 * Calculate incremental speed travelled from previous step.
 * Resets incremental speed to speed if dt=0.
 * (This version is specialised to correct sign)
 * 
 * @param dt Time step (s)
 */
  virtual void calc_incremental_speed(const double dt);
};

class ElementStat
{
public:
/** 
 * Constructor.  Initialises all to zero.
 * 
 */
  ElementStat():
    TimeStarted(-1.0),
    TimeElapsed(0.0),
    TimeRemaining(0.0),
    TimePlanned(0.0),
    gradient(0.0),
    initialised(false)
    {
    };

  double TimeStarted;
  double TimeElapsed;
  double TimeRemaining;
  double TimePlanned;
  double gradient;

  DistanceRemainingStat remaining_effective;
  DistanceRemainingStat remaining;
  DistancePlannedStat planned;
  DistanceTravelledStat travelled;

  GlideResult solution_planned;
  GlideResult solution_travelled;
  GlideResult solution_remaining;

/** 
 * Calculate element times
 * 
 * @param ts Start time of this element (s)
 * @param state Aircraft state (to access time)
 */
  void set_times(const double ts, 
                 const AIRCRAFT_STATE& state);

/** 
 * Calculate element speeds.  Incremental speeds are
 * held at bulk speeds within first minute of elapsed time.
 *
 * @param dt Time step of sample (s)
 */
  void calc_speeds(const double dt);

/** 
 * Reset to uninitialised state, to supress calculation
 * of incremental speeds.
 */
  void reset();

  friend std::ostream& operator<< (std::ostream& o, 
                                   const ElementStat& es);

private:
  bool initialised;
};



class TaskStats 
{
public:
/** 
 * Constructor.  Initialises all to zero.
 * 
 */
  TaskStats():
    Time(0.0),
    cruise_efficiency(1.0),
    glide_required(0.0),
    mc_best(0.0),
    distance_nominal(0.0),
    distance_max(0.0),
    distance_min(0.0),
    distance_scored(0.0)
    {};

  ElementStat total;
  ElementStat current_leg;

  double Time;

  // calculated values
  double glide_required;
  double cruise_efficiency;
  double mc_best;

  double distance_nominal;
  double distance_max;
  double distance_min;
  double distance_scored;

/** 
 * Reset each element (for incremental speeds).
 * 
 */
  void reset();

  friend std::ostream& operator<< (std::ostream& o, 
                                   const TaskStats& ts);
};


#endif
