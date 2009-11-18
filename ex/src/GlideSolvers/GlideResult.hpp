#ifndef GLIDERESULT_HPP
#define GLIDERESULT_HPP

#include <iostream>
#include "Navigation/Geometry/GeoVector.hpp"

struct GlideState;

struct GlideResult {

  enum GlideResult_t {
    RESULT_OK = 0,              /**< Solution is achievable */
    RESULT_PARTIAL,             /**< Solution is partially achievable */
    RESULT_WIND_EXCESSIVE,      /**< Wind is too strong to allow progress */
    RESULT_MACCREADY_INSUFFICIENT, /**< Expected climb rate is too low to allow progress */
    RESULT_NOSOLUTION           /**< Solution not computed or algorithm failed */
  };

/** 
 * Dummy constructor for null result.  Used as default
 * return value for failed/trivial tasks
 * 
 * @return Initialised null result
 */
  GlideResult():
    Vector(0,0),
    DistanceToFinal(0.0),
    CruiseTrackBearing(0.0),
    VOpt(0.0),
    HeightClimb(0.0),
    HeightGlide(0.0),
    TimeElapsed(0.0),
    TimeVirtual(0.0),
    AltitudeDifference(0.0),
    EffectiveWindSpeed(0.0),
    EffectiveWindAngle(0.0),
    Solution(RESULT_NOSOLUTION)
    {
      // default is null result
    }

/** 
 * Constructor with partial initialisation for a particular
 * task.  This copies task information so that the resulting instance
 * contains everything required for navigation and the GlideState 
 * instance can be destroyed.
 * 
 * @param task Task for which glide result will be calculated
 * @param V Optimal speed to fly
 * 
 * @return Blank glide result
 */
  GlideResult(const GlideState &task, 
               const double V);

  GeoVector Vector;             /**< Distance/bearing of task achievable */
  double DistanceToFinal;       /**< Distance to go before final glide (m) */
  double CruiseTrackBearing;    /**< Track bearing in cruise for optimal drift compensation (deg true) */
  double VOpt;                  /**< Optimal speed to fly in cruise (m/s) */
  double HeightClimb;           /**< Height to be climbed (m) */
  double HeightGlide;           /**< Height that will be glided (m) */
  double TimeElapsed;           /**< Time to complete task (s) */
  double TimeVirtual;           /**< Equivalent time to recover glided height (s) at MC */
  double AltitudeDifference;    /**< Height above/below final glide for this task (m) */
  double EffectiveWindSpeed;    /**< (internal) */
  double EffectiveWindAngle;    /**< (internal) */
  GlideResult_t Solution;       /**< Solution validity */

/** 
 * Calculate cruise track bearing from internal variables.
 * This is expensive so is only done on demand.
 */
  void calc_cruise_bearing();

/** 
 * Check whether aircraft can finish this task without
 * further climb.
 * 
 * @return True if aircraft is at or above final glide
 */
  bool is_final_glide() const {
    return (DistanceToFinal==0.0);
  }

/** 
 * Check whether task is partially achievable.  It will
 * fail if the wind is excessive for the current MC value.
 * 
 * @return True if task is at least partially achievable
 */
  bool ok_or_partial() const {
    return (Solution == RESULT_OK)
      || (Solution == RESULT_PARTIAL);
  }

/** 
 * Check whether task is entirely achievable on final glide.
 * 
 * @return True if target is reachable without further climb
 */
  bool glide_reachable() const {
    return (Solution==RESULT_OK) &&
      (AltitudeDifference>=0) &&
      (HeightClimb==0);
  }

/** 
 * Adds another GlideResult to this.  This is used to 
 * accumulate GlideResults for a sequence of task segments.
 * The order is important.
 * 
 * @param s2 The other glide result segment
 */
  void add(const GlideResult &s2);

  double calc_vspeed(const double mc);

  double glide_angle_ground() const;

  friend std::ostream& operator<< (std::ostream& o, 
                                   const GlideResult& gl);

};

#endif
