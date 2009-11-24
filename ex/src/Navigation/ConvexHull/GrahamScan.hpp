#ifndef GRAHAM_SCAN_HPP
#define GRAHAM_SCAN_HPP

#include <list>

#include "Navigation/SearchPointVector.hpp"

/**
 * Class used to build convex hulls from vector.  This ensures
 * the returned vector is closed, and may prune points.
 */
class GrahamScan
{
public :
/** 
 * Constructor.  Note that this class should be used temporarily only
 * 
 * @param sps Input vector of points (may be unordered)
 */
  GrahamScan(const SearchPointVector& sps);

/** 
 * Perform convex hull transformation
 * 
 * @param changed If supplied, will return status as to whether input vector was altered (pruned) or not
 * 
 * @return Vector representing convex hull of input
 */
  SearchPointVector prune_interior(bool *changed=NULL);
private :
  void partition_points();
  void build_hull();
  void build_half_hull( std::vector< SearchPoint* > input,
                        std::vector< SearchPoint* > &output,
                        int factor );
  double direction( const GEOPOINT& p0,
                    const GEOPOINT& p1,
                    const GEOPOINT& p2 );
  std::list< SearchPoint > raw_points;
  SearchPoint *left;
  SearchPoint *right;
  std::vector< SearchPoint* > upper_partition_points;
  std::vector< SearchPoint* > lower_partition_points;
  std::vector< SearchPoint* > lower_hull;
  std::vector< SearchPoint* > upper_hull;
  const SearchPointVector &raw_vector;
};


#endif
