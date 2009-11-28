#ifndef SEARCH_POINT_HPP
#define SEARCH_POINT_HPP

#include "GeoPoint.hpp"
#include "Navigation/Flat/FlatGeoPoint.hpp"
#include "Navigation/ReferencePoint.hpp"

class TaskProjection;

/**
 * Class used to hold a geodetic point, its projected integer form and
 * whether or not the point is a virtual point or an actual search point.
 * The 'virtuality' of this object is currently not used. 
 */
class SearchPoint: public ReferencePoint {
public:

/** 
 * Constructor
 * 
 * @param loc Location of search point
 * @param tp Projection used
 * @param _actual Whether search point is real or virtual
 */
  SearchPoint(const GEOPOINT &loc, const TaskProjection& tp,
    bool _actual=false);

/** 
 * Calculate projected value of geodetic coordinate
 * 
 * @param tp Projection used
 */
  void project(const TaskProjection& tp);

/** 
 * Accessor for flat projected coordinate
 * 
 * 
 * @return Flat projected coordinate
 */
  const FLAT_GEOPOINT& get_flatLocation() const {
    return flatLocation;
  };

/** 
 * Test whether two points are coincident (by their geodetic coordinates)
 * 
 * @param sp Point to compare with
 * 
 * @return True if points coincident
 */
  bool equals(const SearchPoint& sp) const;

/** 
 * Calculate flat earth distance between two points
 * 
 * @param sp Point to measure distance from
 * 
 * @return Distance in projected units
 */
  unsigned flat_distance(const SearchPoint& sp) const;

/** 
 * Rank two points according to longitude, then latitude
 * 
 * @param sp Point to compare to
 * 
 * @return True if this point is further left (or if equal, lower) than the other
 */
  bool sort (const SearchPoint &other) const;

private:
  FLAT_GEOPOINT flatLocation;
  bool actual;
//  double saved_rank;
};


#endif
