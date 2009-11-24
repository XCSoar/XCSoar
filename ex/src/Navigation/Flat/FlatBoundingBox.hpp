#ifndef FLATBOUNDINGBOX_HPP
#define FLATBOUNDINGBOX_HPP

#include "FlatGeoPoint.hpp"
#include "FlatRay.hpp"
#include "Navigation/TaskProjection.hpp"
#include "Navigation/Geometry/BoundingBoxDistance.hpp"
#include <algorithm>

/**
 * Structure defining 2-d integer projected coordinates defining
 * a lower left and upper right bounding box.
 * For use in kd-tree storage of 2-d objects.
 */
class FlatBoundingBox {
public:
/** 
 * Constructor given bounds
 * 
 * @param ll Lower left location
 * @param ur Upper right location
 */
  FlatBoundingBox(const FLAT_GEOPOINT &ll,
                  const FLAT_GEOPOINT &ur):
    bb_ll(ll.Longitude,ll.Latitude),
    bb_ur(ur.Longitude,ur.Latitude) {};

/** 
 * Constructor given center point and radius
 * (produces a box enclosing a circle of given radius at center point)
 * 
 * @param loc Location of center point
 * @param range Radius in projected units
 */
  FlatBoundingBox(const FLAT_GEOPOINT &loc,
                  const unsigned range=0):
    bb_ll(loc.Longitude-range,loc.Latitude-range),
    bb_ur(loc.Longitude+range,loc.Latitude+range) 
  {

  }

/** 
 * Calculate non-overlapping distance from one box to another.
 * 
 * @param f That box
 * 
 * @return Distance in projected units (or zero if overlapping)
 */
  unsigned distance(const FlatBoundingBox &f) const;

  /** 
   * Function object used by kd-tree to index coordinates 
   */
  struct kd_get_bounds {
    typedef int result_type; /**< Used by kd-tree */
/** 
 * Retrieve coordinate value given coordinate index and object
 *
 * @param d Object being stored in kd-tree
 * @param k Index of coordinate
 * 
 * @return Coordinate value
 */
    int operator() ( const FlatBoundingBox &d, const unsigned k) const {
      switch(k) {
      case 0:
        return d.bb_ll.Longitude;
      case 1:
        return d.bb_ll.Latitude;
      case 2:
        return d.bb_ur.Longitude;
      case 3:
        return d.bb_ur.Latitude;
      };
      return 0; 
    };
  };

  /**
   * Distance metric function object used by kd-tree.  This specialisation
   * allows for overlap; distance is zero with overlap, otherwise the minimum
   * distance between two regions.
   */
  struct kd_distance {
    typedef BBDist distance_type; /**< Distance operator for overlap functionality */

/** 
 * \todo document this!
 *
 * @param a
 * @param b
 * @param dim
 *
 * @return Distance on axis
 */
    distance_type operator() (const int &a, const int &b, 
                              const size_t dim) const 
      {
        int val = 0;
        if (dim<2) {
          val= std::max(b-a,0);
        } else {
          val= std::max(a-b,0);
        }
        return BBDist(dim,val);
      }
  };

/** 
 * Test ray-box intersection
 * 
 * @param ray Ray to test for intersection
 * 
 * @return True if ray intersects with this bounding box
 */
  bool intersects(const FlatRay& ray) const;

private:
  FLAT_GEOPOINT bb_ll;
  FLAT_GEOPOINT bb_ur;

  /** @link dependency */
  /*#  BBDist lnkBBDist; */
};

#endif
