#ifndef AIRSPACE_HPP
#define AIRSPACE_HPP

#include "Navigation/FlatBoundingBox.hpp"
#include "AbstractAirspace.hpp"
#include "Util/GenericVisitor.hpp"

class Airspace: 
  public FlatBoundingBox,
  public BaseVisitable<>
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

  bool intersects(const FlatRay& ray) const;
  bool inside(const AIRCRAFT_STATE &loc) const;
  bool intersects(const GEOPOINT& g1, const GeoVector &vec) const;

  void destroy();
private:

  /**
   * @supplierCardinality 0..1 
   */
  AbstractAirspace *pimpl_airspace;

public:
#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& f, 
                                   const Airspace& ts);
#endif
  void Accept(BaseVisitor &v) const;
};

#endif
