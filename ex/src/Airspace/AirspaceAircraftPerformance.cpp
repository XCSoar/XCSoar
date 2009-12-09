#include "AirspaceAircraftPerformance.hpp"

fixed 
AirspaceAircraftPerformance::solution(const fixed& distance,
                                      const fixed& dh) const
{
  // can at least descend as fast as s_ld
  fixed mod_descent_rate = max(descent_rate, s_ld);

  const fixed t_cruise = distance/v_ld;
  const fixed h_descent = dh-t_cruise*s_ld;

  if (!fabs(h_descent)) {
    return t_cruise;
  } 
  if (h_descent>0) {
    // descend steeper than best glide
    if (!positive(mod_descent_rate)) {
      return -fixed_one;
    }
    const fixed t_descent = h_descent/mod_descent_rate;
    return max(t_cruise, t_descent);
  } else {
    // require climb
    if (!positive(climb_rate)) {
      return -fixed_one;
    }
    const fixed t_climb = -h_descent/climb_rate;
    return t_cruise+t_climb;
  }
}
                                      

/*
TODO: write a sorter/visitor so that we can visit airspaces in increasing
  order of arrival time (plus other criteria). 
 */
