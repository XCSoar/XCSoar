#ifndef AIRSPACE_HPP
#define AIRSPACE_HPP

#include "FlatBoundingBox.hpp"
#include "AbstractAirspace.hpp"

class Airspace: public FlatBoundingBox 
{
public:

  Airspace(AbstractAirspace& airspace,
           const TaskProjection& tp);

  // used by query searches (this is a virtual airspace)
  Airspace(const GEOPOINT&loc, const TaskProjection& task_projection, const
    double range=0.0):
    FlatBoundingBox(task_projection.project(loc),
                    task_projection.project_range(loc,range)),
    pimpl_airspace(NULL)
  {
  };
  Airspace(const GEOPOINT &ll, 
           const GEOPOINT &ur,
           const TaskProjection& task_projection):
    FlatBoundingBox(task_projection.project(ll),
                    task_projection.project(ur)), 
    pimpl_airspace(NULL)
  {
  };

  virtual void print(std::ostream &f, const TaskProjection &task_projection) const;

  bool inside(const AIRCRAFT_STATE &loc) const;

  void destroy();
private:

  /**
   * @supplierCardinality 0..1 
   */
  AbstractAirspace *pimpl_airspace;
};

#endif
