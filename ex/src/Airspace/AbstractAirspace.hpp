#ifndef ABSTRACTAIRSPACE_HPP
#define ABSTRACTAIRSPACE_HPP

#include "Util/GenericVisitor.hpp"
#include "Navigation/Flat/FlatBoundingBox.hpp"
#include "Navigation/Aircraft.hpp"
#include "Navigation/Geometry/GeoVector.hpp"

class AbstractAirspace:
  public BaseVisitable<>
{
public:
  /** 
   * Compute bounding box enclosing the airspace.  Rounds up/down
   * so discretisation ensures bounding box is indeed enclosing.
   * 
   * @param task_projection Projection used for flat-earth representation
   * 
   * @return Enclosing bounding box
   */
  virtual const FlatBoundingBox 
    get_bounding_box(const TaskProjection& task_projection) const = 0;

  /** 
   * Checks whether an aircraft is inside the airspace.
   * This is slow because it uses geodesic calculations
   * 
   * @param loc State about which to test inclusion
   * 
   * @return true if aircraft is inside airspace boundary
   */
  virtual bool inside(const AIRCRAFT_STATE &loc) const = 0;

  /** 
   * Checks whether a line intersects with the airspace.
   * Can be approximate by using flat-earth representation internally.
   * 
   * @param g1 Location of origin of search vector
   * @param vec Line from origin
   * @param tp Projection used by flat-earth representation
   * 
   * @return true if the line intersects the airspace
   */
  virtual bool intersects(const GEOPOINT& g1, 
                          const GeoVector &vec,
                          const TaskProjection& tp) const = 0;

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& f, 
                                   const AbstractAirspace& as);
#endif
public:
  DEFINE_VISITABLE()
};

#endif
