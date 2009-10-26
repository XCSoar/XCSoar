#ifndef AIRSPACES_HPP
#define AIRSPACES_HPP

#include <kdtree++/kdtree.hpp>
#include "Airspace.hpp"
#include <iostream>

class Airspaces {
public:
  Airspaces(const TaskProjection& _task_projection):
    task_projection(_task_projection)
    {
      fill_default();
    };

  void scan_nearest(const AIRCRAFT_STATE &state,
    const bool do_report) const;
  void scan_range(const AIRCRAFT_STATE &state, const double &range,
    const bool do_report) const;

private:
  void fill_default();

  typedef KDTree::KDTree<4, 
                         Airspace, 
                         FlatBoundingBox::kd_get_bounds,
                         FlatBoundingBox::kd_distance
                         > AirspaceTree;

  AirspaceTree airspace_tree;
  const TaskProjection& task_projection;

  /** @link dependency */
  /*#  Airspace lnkAirspace; */
};

#endif
