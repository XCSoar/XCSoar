#ifndef AIRSPACES_HPP
#define AIRSPACES_HPP

#include <kdtree++/kdtree.hpp>
#include "Airspace.hpp"
#include "Navigation/TaskProjectionClient.hpp"

class AirspaceVisitor;

class Airspaces: public TaskProjectionClient 
{
public:
  Airspaces(const TaskProjection& _task_projection):
    TaskProjectionClient(_task_projection)
    {
    };

  const std::vector<Airspace> scan_nearest(const AIRCRAFT_STATE &state) const;

  const std::vector<Airspace> scan_range(const AIRCRAFT_STATE &state, 
                                         const double range) const;

  void visit_within_range(const GEOPOINT &loc, 
                          const double range,
                          AirspaceVisitor& visitor) const;

  void visit_intersecting(const GEOPOINT &loc, 
                          const GeoVector &vec,
                          AirspaceVisitor& visitor) const;

  std::vector<Airspace> find_inside(const AIRCRAFT_STATE &state) const;

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
