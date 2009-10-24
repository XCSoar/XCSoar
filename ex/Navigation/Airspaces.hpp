#ifndef AIRSPACES_HPP
#define AIRSPACES_HPP

#include <kdtree++/kdtree.hpp>
#include "FlatBoundingBox.hpp"
#include <iostream>

class Airspaces {
public:
  Airspaces()
    {
      fill_default();
    };

  void scan_nearest(const FLAT_GEOPOINT &loc) const;
  void scan_range(const FLAT_GEOPOINT &loc, const unsigned &range);

private:
  void fill_default();

  typedef KDTree::KDTree<2, 
                         FlatBoundingBox, 
                         FlatBoundingBox::kd_get_bounds,
                         FlatBound::kd_distance, 
                         FlatBound::kd_less > FlatBoundingBoxTree;

  FlatBoundingBoxTree airspace_tree;
};

#endif
