#ifndef AIRSPACES_HPP
#define AIRSPACES_HPP

#include <kdtree++/kdtree.hpp>
#include "Airspace.hpp"
#include <iostream>
#include "BaseTask/TaskProjectionClient.hpp"

class Airspaces: public TaskProjectionClient 
{
public:
  Airspaces(const TaskProjection& _task_projection):
    TaskProjectionClient(_task_projection)
    {
    };

  void scan_nearest(const AIRCRAFT_STATE &state,
    const bool do_report) const;
  void scan_range(const AIRCRAFT_STATE &state, const double &range,
    const bool do_report) const;

  std::vector<Airspace> find_inside(const AIRCRAFT_STATE &state,
    const bool do_report) const;

  void optimise();

  void insert(AbstractAirspace& asp);

private:

  typedef KDTree::KDTree<4, 
                         Airspace, 
                         FlatBoundingBox::kd_get_bounds,
                         FlatBoundingBox::kd_distance
                         > AirspaceTree;

  AirspaceTree airspace_tree;

  /** @link dependency */
  /*#  Airspace lnkAirspace; */
};

#endif
