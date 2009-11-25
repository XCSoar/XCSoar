#ifndef GLIDESTATE_HPP
#define GLIDESTATE_HPP

#include "Navigation/Geometry/GeoVector.hpp"

struct AIRCRAFT_STATE;
struct GEOPOINT;

/**
 * Class used to define a glide/navigation task
 */
struct GlideState 
{

/** 
 * Dummy task constructor.  Typically used for synthetic glide
 * tasks.  Where there are real targets, the other constructors should
 * be used instead.
 * 
 * @param vector Specified vector for task
 * @param htarget Height of target (m above MSL)
 * @param aircraft Aircraft state
 * 
 * @return Initialised glide task
 */
  GlideState(const GeoVector &vector,
              const double htarget,
              const AIRCRAFT_STATE &aircraft);

/** 
 * Constructor for glide task from aircraft to a point.
 * 
 * @param aircraft State of aircraft at start
 * @param target Location of end point
 * @param htarget Height of end point (m above MSL)
 * 
 * @return Initialised glide task for task remaining
 */
  GlideState(const AIRCRAFT_STATE &aircraft,
              const GEOPOINT& target,
              const double htarget);

  GeoVector Vector;             /**< Distance/bearing of task */
  double MinHeight;             /**< Height (m above MSL) of end */
  double WindDirection;         /**< Direction of wind (deg True) */
  double AltitudeDifference;    /**< Aircraft height less target height */

/** 
 * Calculate internal quantities to reduce computation time
 * by clients of this class
 * 
 * @param aircraft Aircraft state
 */
  void calc_speedups(const AIRCRAFT_STATE &aircraft);

/** 
 * Calculates average cross-country speed from effective 
 * cross-country speed (accounting for wind)
 * 
 * @param Veff Effective cruise speed (m/s)
 * 
 * @return Average cross-country speed (m/s)
 */
  double calc_ave_speed(const double Veff) const;

/** 
 * Calculate distance a circling aircraft will drift
 * in a given time
 * 
 * @param t_climb Time spent in climb (s)
 * 
 * @return Distance (m) of drift
 */
  double drifted_distance(const double t_climb) const;

  double EffectiveWindSpeed;    /**< (internal use) */
  double EffectiveWindAngle;    /**< (internal use) */
  double wsq_;                  /**< (internal use) */
  double dwcostheta_;           /**< (internal use) */
};

#endif
