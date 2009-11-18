#include "FlatRay.hpp"

/**
 * Checks whether two lines 
 * intersect or not
 * @see http://local.wasp.uwa.edu.au/~pbourke/geometry/lineline2d/
 * adapted from line_line_intersection
 */
double 
FlatRay::intersects (const FlatRay &oray) const
{
  const int denom = vector.cross(oray.vector);
  if (denom == 0) {
    // lines are parallel
    return -1;
  }
  const FLAT_GEOPOINT delta = point-oray.point;
  const int ua = vector.cross(delta);
  if ((ua<0) || (ua>denom)) {
    // outside first line
    return -1;
  } 
  const int ub = oray.vector.cross(delta);
  if ((ub<0) || (ub>denom)) {
    // outside second line
    return -1;
  }  

  // inside both lines
  return ((double)ua)/denom;
}


FLAT_GEOPOINT
FlatRay::parametric(const double t) const
{
  FLAT_GEOPOINT p = point;
  p.Longitude += (int)(vector.Longitude*t);
  p.Latitude += (int)(vector.Latitude*t);
  return p;
}
