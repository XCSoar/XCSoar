#ifndef AIRSPACE_AIRCRAFT_PERFORMANCE_HPP
#define AIRSPACE_AIRCRAFT_PERFORMANCE_HPP

#include "Math/fixed.hpp"

/**
 *  Class used for simplified/idealised performace
 *  of aircraft speed as a function of glide slope.
 *  \todo method to derive this data from glide polar
 *  \todo specialisation based on recent trajectory
 */
class AirspaceAircraftPerformance {
public:
/** 
 * Constructor.  Initialises current to experimental values
 * 
 * \todo use something more sensible?
 */
  AirspaceAircraftPerformance():v_ld(30.0),s_ld(2.0),
                                climb_rate(10.0),
                                descent_rate(10.0)
    {};

/** 
 * Return maximum speed achievable by this model
 * 
 * @return Speed (m/s)
 */
  fixed max_speed() const {
    return v_ld;
  };

/** 
 * Find minimum intercept time to a point
 * 
 * @param distance Distance to point (m)
 * @param dh Height of observer from point (m)
 * 
 * @return Time to intercept (s) or -1 if failed
 */
  virtual fixed solution_general(const fixed& distance,
                                 const fixed& dh) const;

/** 
 * Find time to intercept a target with a height band, set distance
 * 
 * @param distance Lateral distance to travel (m)
 * @param altitude Altitude of observer (m)
 * @param base Height of base (m)
 * @param top  Height of top (m)
 * @param intercept_alt If intercept possible, this is the soonest height
 * 
 * @return Time of intercept (s)
 */
  fixed solution_vertical(const fixed& distance,
                          const fixed& altitude,
                          const fixed& base,
                          const fixed& top,
                          fixed& intercept_alt) const;

/** 
 * Find time to intercept a target with a distance band, set height 
 * 
 * @param distance_min Min distance to travel (m)
 * @param distance_max Max distance to travel (m)
 * @param altitude Altitude of observer (m)
 * @param h  Height of target (m)
 * @param intercept_distance If intercept possible, this is the distance to the soonest point
 * 
 * @return Time of intercept (s)
 */
  fixed solution_horizontal(const fixed& distance_min,
                            const fixed& distance_max,
                            const fixed& altitude,
                            const fixed& h,
                            fixed& intercept_distance) const;

private:
  virtual bool solution_exists(const fixed& distance_min,
                               const fixed& distance_max,
                               const fixed& h_min,
                               const fixed& h_max) const;

  fixed v_ld;
  fixed s_ld;
  fixed climb_rate;
  fixed descent_rate;
};

#endif
