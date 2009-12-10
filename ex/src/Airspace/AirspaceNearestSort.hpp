#ifndef AIRSPACE_NEAREST_SORT_HPP
#define AIRSPACE_NEAREST_SORT_HPP

#include "Navigation/GeoPoint.hpp"
#include "AirspacePredicate.hpp"
#include <queue>
#include "Navigation/Aircraft.hpp"
#include "Airspace.hpp"

class AirspaceVisitor;
class Airspaces;

/**
 *  Class to sort nearest airspaces by distance to closest point.
 */
class AirspaceNearestSort
{
public:
/** 
 * Constructor
 * 
 * @param state State of aircraft
 * @param condition Additional condition to be placed on queries (default always true)
 * 
 * @return Initialised object
 */
  AirspaceNearestSort(const AIRCRAFT_STATE &state,
                      const AirspacePredicate &condition=AirspacePredicate::always_true):
    m_state(state),
    m_location(state.Location),
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
 * Metric defining sort criteria
 * 
 * @param a Airspace to determine metric of
 * 
 * @return Positive value indicating rank (low best), negative indicates invalid
 */
  virtual fixed metric(const AbstractAirspace &a) const;

/** 
 * Set reversal of sorting order
 * 
 * @param set New value 
 */
  void set_reverse(bool set) {
    m_reverse = set;
  }

protected:

  const AIRCRAFT_STATE &m_state; /**< State of observer for queries */
  const GEOPOINT &m_location; /**< Location of observer for queries */
  const AirspacePredicate &m_condition; /**< Condition to be applied to queries */

private:

  typedef std::pair<fixed, Airspace> Item;

  /**
   * Function object used to rank intercepts by vector parameter t(0,1)
   */
  struct Rank : public std::binary_function<Item, Item, bool> {
    bool operator()(const Item& x, const Item& y) const {
      return x.first > y.first;
    }
  };

  typedef std::priority_queue<Item, std::vector<Item>, Rank> Queue;

  bool m_reverse;

};


#endif
