#ifndef AIRSPACE_INTERCEPT_SOLUTION_HPP
#define AIRSPACE_INTERCEPT_SOLUTION_HPP

#include "Navigation/GeoPoint.hpp"
#include "Math/fixed.hpp"

/**
 *  Structure to hold data for intercepts between aircraft and airspace.
 *  (interior or exterior)
 *
 */
struct AirspaceInterceptSolution {
  GeoPoint location; /**< Location of intercept point */
  fixed distance;  /**< Distance from observer to intercept point (m) */
  fixed altitude; /**< Altitude AMSL (m) of intercept point */
  fixed elapsed_time; /**< Estimated time (s) for observer to reach intercept point */

  /**
   *  Constructor, initialises to invalid solution
   */
  AirspaceInterceptSolution():
    distance(-fixed_one),
    altitude(-fixed_one),
    elapsed_time(-fixed_one) {};

/** 
 * Determine whether this solution is valid
 * 
 * @return True if solution is valid
 */
  bool valid() const {
    return !negative(elapsed_time);
  };
};

#endif
