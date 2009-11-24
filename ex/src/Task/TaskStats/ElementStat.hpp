#ifndef ELEMENT_STAT_HPP
#define ELEMENT_STAT_HPP

#include "GlideSolvers/GlideResult.hpp"
#ifdef DO_PRINT
#include <iostream>
#endif
#include "DistanceStat.hpp"

struct AIRCRAFT_STATE;

/**
 * Common task element statistics.  Used because we separately want to
 * track overall task statistics as well as that of the current leg.
 */
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

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, 
                                   const ElementStat& es);
#endif

private:
  bool initialised;
};


#endif
