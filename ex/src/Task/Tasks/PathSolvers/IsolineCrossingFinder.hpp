#ifndef ISOLINECROSSINGFINDER_HPP
#define ISOLINECROSSINGFINDER_HPP

#include "Util/ZeroFinder.hpp"

class GeoEllipse;
class AATPoint;

/**
 *  Calculate where Isoline ellipse crosses border of observation zone
 */
class IsolineCrossingFinder:
  public ZeroFinder
{
public:
/** 
 * Constructor.  After construction, call solve() to perform the search.
 * 
 * @param _aap AATPoint for which to test OZ inclusion
 * @param _ell GeoEllipse representing the isoline
 * @param xmin Min parameter of search
 * @param xmax Max parameter of search
 * 
 * @return Partially initialised object
 */
  IsolineCrossingFinder(const AATPoint& _aap,
                        const GeoEllipse &_ell,
                        const double xmin, 
                        const double xmax);
  double f(const double t);
  bool valid(const double x);

/** 
 * Search for parameter value of isoline intersecting the OZ boundary
 * within min/max parameter range.
 * 
 * @return Parameter value of isoline intersection
 */
  double solve();
private:
  const AATPoint &aap;
  const GeoEllipse &ell;
};

#endif
