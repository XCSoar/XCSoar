#ifndef AIRSPACE_INTERSECT_SORT_HPP
#define AIRSPACE_INTERSECT_SORT_HPP

#include <queue>
#include "Math/fixed.hpp"
#include "Navigation/GeoPoint.hpp"
#include "AbstractAirspace.hpp"

/**
 * Utility class to sort airspaces in ascending order of vector parameter (0,1)
 */
class AirspaceIntersectSort {
public:

/** 
 * Constructor
 * 
 * @param start Location of start point
 * @param end Location of end point
 */
  AirspaceIntersectSort(const GEOPOINT &start,
                        const GEOPOINT &end,
                        const AbstractAirspace &the_airspace):
    m_start(start),
    m_end(end),
    m_airspace(&the_airspace) {}

/** 
 * Add point to queue
 * 
 * @param t Ray parameter [0,1]
 * @param p Point of intersection
 */
  void add(const fixed t, const GEOPOINT &p);

/** 
 * Determine if no points are found
 * 
 * @return True if no points added
 */
  bool empty() const;

/** 
 * Return closest intercept point (or location if inside)
 * 
 * @param p Point to set if any
 * 
 * @return True if an intercept was found
 */
  bool top(GEOPOINT &p) const;

/** 
 * Return vector of pairs of enter/exit intersections.
 * If fill_end is true, will add virtual closure point to last intercept at end.
 * Otherwise it will add the entry point twice as the last entry
 * 
 * @param fill_end 
 * 
 * @return vector
 */
  AirspaceIntersectionVector all(const bool fill_end=false);

private:
  typedef std::pair<fixed,GEOPOINT> Intersection;

  /**
   * Function object used to rank intercepts by vector parameter t(0,1)
   */
  struct Rank : public std::binary_function<Intersection, Intersection, bool> {
    bool operator()(const Intersection& x, const Intersection& y) const {
      return x.first > y.first;
    }
  };

  std::priority_queue<Intersection, std::vector<Intersection>, Rank> m_q;

  const GEOPOINT& m_start;
  const GEOPOINT& m_end;
  const AbstractAirspace *m_airspace;
};

#endif
