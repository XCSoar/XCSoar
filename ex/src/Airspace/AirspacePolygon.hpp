#ifndef AIRSPACEPOLYGON_HPP
#define AIRSPACEPOLYGON_HPP

#include "AbstractAirspace.hpp"
#include "Navigation/SearchPointVector.hpp"
#include <vector>

/**
 * General polygon form airspace
 *
 */
class AirspacePolygon: public AbstractAirspace 
{
public:
  /** 
   * Constructor.  For testing, pts vector is a cloud of points,
   * and the airspace polygon becomes the convex hull border.
   * \todo remove convex hack for testing
   *
   * @param pts Vector representing border
   *
   * @return Initialised airspace object
   */
  AirspacePolygon(const std::vector<GEOPOINT>& pts);

  /** 
   * Compute bounding box enclosing the airspace.  Rounds up/down
   * so discretisation ensures bounding box is indeed enclosing.
   * 
   * @param task_projection Projection used for flat-earth representation
   * 
   * @return Enclosing bounding box
   */
  const FlatBoundingBox get_bounding_box(const TaskProjection& task_projection);

/** 
 * Get arbitrary center or reference point for use in determining
 * overall center location of all airspaces
 * 
 * @return Location of reference point
 */
  const GEOPOINT get_center();

  /** 
   * Checks whether an aircraft is inside the airspace.
   * This is slow because it uses geodesic calculations
   * 
   * @param loc State about which to test inclusion
   * 
   * @return true if aircraft is inside airspace boundary
   */
  bool inside(const AIRCRAFT_STATE &loc) const;

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
  bool intersects(const GEOPOINT& g1, 
                  const GeoVector &vec,
                  const TaskProjection& tp) const;

private:
  SearchPointVector border;

/** 
 * Project border.
 */
  virtual void project(const TaskProjection& tp);

public:
#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& f, 
                                   const AirspacePolygon& as);
#endif
  DEFINE_VISITABLE()
};

#endif
