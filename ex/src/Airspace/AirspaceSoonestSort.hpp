#ifndef AIRSPACE_SOONEST_SORT_HPP
#define AIRSPACE_SOONEST_SORT_HPP

#include "AirspaceNearestSort.hpp"
#include "AirspaceAircraftPerformance.hpp"

class AirspaceSoonestSort:
  public AirspaceNearestSort
{
public:
  AirspaceSoonestSort(const AIRCRAFT_STATE &state,
                      const AirspaceAircraftPerformance &perf,
                      const fixed max_time= 1.0e6,
                      const AirspacePredicate &condition=AirspacePredicate::always_true):
    AirspaceNearestSort(state, condition),
    m_perf(perf),
    m_max_time(max_time) {};

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
