#ifndef GRAHAM_SCAN_HPP
#define GRAHAM_SCAN_HPP

#include <vector>
#include <list>

#include "BaseTask/SearchPoint.hpp"

class GrahamScan
{
public :
  GrahamScan(const std::vector<SearchPoint>& sps);
  std::vector<SearchPoint> prune_interior(bool *changed=NULL);
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
  const std::vector< SearchPoint > &raw_vector;
};


#endif
