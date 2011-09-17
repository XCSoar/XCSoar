#ifndef AIRSPACE_NEAREST_SORT_HPP
#define AIRSPACE_NEAREST_SORT_HPP

#include "Navigation/GeoPoint.hpp"
#include "Predicate/AirspacePredicate.hpp"
#include <queue>
#include "Navigation/Aircraft.hpp"
#include "Airspace.hpp"
#include "AirspaceInterceptSolution.hpp"

class AirspaceVisitor;
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

  bool m_reverse;

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
    m_condition(condition),
    m_reverse(false) {};

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
 * 
 * Visit all nearest in ascending order
 * 
 * @param airspaces Airspaces to search
 * @param visitor Visitor to apply to matches, in sorted order
 * @param range Maximum range of search
 */
  void visit_sorted(const Airspaces &airspaces,
                    AirspaceVisitor &visitor,
                    const fixed range);

/** 
 * Compute complete or partial solution as required to this sort strategy
 * 
 * @param a Airspace to compute solution for
 * 
 * @return Solution
 */
  virtual AirspaceInterceptSolution solve_intercept(const AbstractAirspace &a) const;

/** 
 * Metric defining sort criteria
 * 
 * @param ais Intercept solution to determine metric of
 * 
 * @return Positive value indicating rank (low best), negative indicates invalid
 */
  virtual fixed metric(const AirspaceInterceptSolution& ais) const;

/** 
 * Set reversal of sorting order
 * 
 * @param set New value 
 */
  void set_reverse(bool set) {
    m_reverse = set;
  }

private:

  void populate_queue(const Airspaces &airspaces,
                      const fixed range);
};


#endif
