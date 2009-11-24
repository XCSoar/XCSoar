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

  double TimeStarted; /**< Time (s) this element was started */
  double TimeElapsed; /**< Time (s) since element was started */
  double TimeRemaining; /**< Time (s) to element completion */
  double TimePlanned; /**< Time (s) of overall element */
  double gradient; /**< Gradient to element completion */

  DistanceRemainingStat remaining_effective; /**< Stats for effective remaining distance of element */
  DistanceRemainingStat remaining; /**< Stats for actual remaining distance of element */
  DistancePlannedStat planned; /**< Stats for overall element distance */
  DistanceTravelledStat travelled; /**< Stats for travelled distance in this element */

  GlideResult solution_planned; /**< Glide solution for planned element */
  GlideResult solution_travelled; /**< Glide solution for travelled element */
  GlideResult solution_remaining; /**< Glide solution for remaining element */

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
