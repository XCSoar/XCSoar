#ifndef AIRSPACE_SOONEST_SORT_HPP
#define AIRSPACE_SOONEST_SORT_HPP

#include "AirspaceNearestSort.hpp"
#include "AirspaceAircraftPerformance.hpp"

/**
 *  Class to sort nearest airspaces according to soonest intercept
 *  possible according to an AircraftPerformanceModel.
 */
class AirspaceSoonestSort:
  public AirspaceNearestSort
{
public:
/** 
 * Constructor
 * 
 * @param state State of aircraft
 * @param perf Aircraft performance model
 * @param max_time Maximum time allowed for results
 * @param condition Additional condition to be placed on queries (default always true)
 * 
 * @return Initialised object
 */
  AirspaceSoonestSort(const AIRCRAFT_STATE &state,
                      const AirspaceAircraftPerformance &perf,
                      const fixed max_time= 1.0e6,
                      const AirspacePredicate &condition=AirspacePredicate::always_true):
    AirspaceNearestSort(state, condition),
    m_perf(perf),
    m_max_time(max_time) {};

/** 
 * Metric function used for sorting.  If returns as negative, 
 * solution will be ignored.
 * 
 * @param a Airspace to calculate metric for
 * 
 * @return Metric value (non-negative low is better)
 */
  virtual fixed metric(const AbstractAirspace &a) const;

/** 
 * Convenience method, calls find_nearest(airspaces, range) with range
 * set according to limit of performance model.
 * 
 * @param airspaces Airspaces to search
 * 
 * @return Soonest arrival time airspace
 */
  const AbstractAirspace* find_nearest(const Airspaces &airspaces);

private:
  const AirspaceAircraftPerformance &m_perf;
  const fixed& m_max_time;
};


#endif
