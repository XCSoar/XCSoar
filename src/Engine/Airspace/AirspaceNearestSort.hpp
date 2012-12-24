#ifndef AIRSPACE_NEAREST_SORT_HPP
#define AIRSPACE_NEAREST_SORT_HPP

#include "Geo/GeoPoint.hpp"
#include "Predicate/AirspacePredicate.hpp"
#include "Airspace.hpp"
#include "AirspaceInterceptSolution.hpp"

#include <queue>

class Airspaces;

/**
 *  Class to sort nearest airspaces by distance to closest point.
 *  Not intended to be called repeatedly; intended as temporary object
 */
class AirspaceNearestSort
{
  typedef std::pair<AirspaceInterceptSolution, Airspace> AirspaceSolutionItem;
  typedef std::pair<fixed, AirspaceSolutionItem> Item;

  /**
   * Function object used to rank intercepts by vector parameter t(0,1)
   */
  struct Rank : public std::binary_function<Item, Item, bool> {
    bool operator()(const Item& x, const Item& y) const {
      return x.first > y.first;
    }
  };

  typedef std::priority_queue<Item, std::vector<Item>, Rank> Queue;

protected:
  const GeoPoint m_location; /**< Location of observer for queries */
  const AirspacePredicate &m_condition; /**< Condition to be applied to queries */

private:
  Queue m_q;

public:
/** 
 * Constructor
 * 
 * @param _location Location of aircraft
 * @param condition Additional condition to be placed on queries (default always true)
 * 
 * @return Initialised object
 */
  AirspaceNearestSort(const GeoPoint _location,
                      const AirspacePredicate &condition=AirspacePredicate::always_true):
    m_location(_location),
    m_condition(condition) {}

/** 
 * Find nearest
 * 
 * @param airspaces Airspaces to search
 * @param range Range of search (m)
 * 
 * @return Nearest match (measured by metric) within range
 */
  const AbstractAirspace* find_nearest(const Airspaces &airspaces,
                                       const fixed range);

/** 
 * Compute complete or partial solution as required to this sort strategy
 * 
 * @param a Airspace to compute solution for
 * 
 * @return Solution
 */
  virtual AirspaceInterceptSolution solve_intercept(const AbstractAirspace &a,
                                                    const TaskProjection &projection) const;

/** 
 * Metric defining sort criteria
 * 
 * @param ais Intercept solution to determine metric of
 * 
 * @return Positive value indicating rank (low best), negative indicates invalid
 */
  virtual fixed metric(const AirspaceInterceptSolution& ais) const;

private:

  void populate_queue(const Airspaces &airspaces,
                      const fixed range);
};


#endif
