#ifndef GEOELLIPSE_HPP
#define GEOELLIPSE_HPP

#include "Navigation/TaskProjection.hpp"
#include "Navigation/Flat/FlatEllipse.hpp"

/**
 * Ellipse in geodesic coordinates, defined by two foci and
 * a point on the ellipse.  Internally uses a flat-earth projection
 * to avoid complex and expensive geodesic calculations.
 */
class GeoEllipse {
public:
/** 
 * Constructor
 * 
 * @param f1 Focus 1
 * @param f2 Focus 2
 * @param p Point on ellipse
 * @param _task_projection Task projection used for internal representation
 */
  GeoEllipse(const GEOPOINT &f1, const GEOPOINT &f2,
             const GEOPOINT &p,
             const TaskProjection &_task_projection): 
    task_projection(_task_projection)
    {
      ell = FlatEllipse(task_projection.fproject(f1),
                        task_projection.fproject(f2),
                        task_projection.fproject(p));
    }

/** 
 * Parametric form of ellipse border
 * 
 * @param t Parameter (0,1)
 * 
 * @return Location of point on ellipse
 */
  GEOPOINT parametric(double t) const {
    FlatPoint fp = ell.parametric(t);
    return task_projection.funproject(fp);
  };

/** 
 * Calculate where a line from the first focus through a point p
 * intersects with the ellipse. 
 * 
 * @param p Origin of point from which to search
 * @param i1 Location of closest intersection point
 * @param i2 Location of furthest intersection point
 * 
 * @return True if line intersects
 */
  bool intersect_extended(const GEOPOINT &p,
                          GEOPOINT &i1,
                          GEOPOINT &i2) const {
    FlatPoint pf = task_projection.fproject(p);
    FlatPoint i1f, i2f;
    if (ell.intersect_extended(pf,i1f,i2f)) {
      i1 = task_projection.funproject(i1f);
      i2 = task_projection.funproject(i2f);
      return true;
    } else {
      return false;
    }
  };
private:
  TaskProjection task_projection;
  FlatEllipse ell;
};


#endif
