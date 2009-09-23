#ifndef GRAHAM_SCAN_HPP
#define GRAHAM_SCAN_HPP

#include <vector>
#include <list>

#include "SearchPoint.hpp"

class GrahamScan
{
public :
  GrahamScan(std::list<SEARCH_POINT>& sps);
  void prune_interior();
private :
  void partition_points();
  void build_hull();
  void build_half_hull( std::vector< SEARCH_POINT* > input,
                        std::vector< SEARCH_POINT* > &output,
                        int factor );
  double direction( const GEOPOINT& p0,
                    const GEOPOINT& p1,
                    const GEOPOINT& p2 );
  std::list< SEARCH_POINT > &raw_points;
  SEARCH_POINT *left;
  SEARCH_POINT *right;
  std::vector< SEARCH_POINT* > upper_partition_points;
  std::vector< SEARCH_POINT* > lower_partition_points;
  std::vector< SEARCH_POINT* > lower_hull;
  std::vector< SEARCH_POINT* > upper_hull;
};


#endif
