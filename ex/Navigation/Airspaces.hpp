#ifndef AIRSPACES_HPP
#define AIRSPACES_HPP

#include <stdio.h>
#include <kdtree++/kdtree.hpp>
#include "FlatBoundingBox.hpp"
#include "BaseTask/TaskProjection.h"
#include <iostream>

class Airspaces {
public:
  Airspaces(const TaskProjection& _task_projection):
    task_projection(_task_projection)
    {
      fill_default();
    };

  void scan_nearest(const GEOPOINT &loc,
    const bool do_report) const;
  void scan_range(const GEOPOINT &loc, const unsigned &range,
    const bool do_report) const;

private:
  void fill_default();

  typedef KDTree::KDTree<4, 
                         FlatBoundingBox, 
                         FlatBoundingBox::kd_get_bounds,
                         FlatBoundingBox::kd_distance
                         > FlatBoundingBoxTree;

  FlatBoundingBoxTree airspace_tree;
  const TaskProjection& task_projection;
};

#endif
