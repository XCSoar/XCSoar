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
 * Find time to reach target
 * 
 * @param distance Lateral distance to travel (m)
 * @param dh Height difference (positive means aircraft is above target), m
 * 
 * @return Time of intercept (s)
 */
  fixed solution(const fixed& distance,
                 const fixed& dh) const;
private:
  fixed v_ld;
  fixed s_ld;
  fixed climb_rate;
  fixed descent_rate;
};

#endif
