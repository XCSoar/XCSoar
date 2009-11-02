#ifndef ISOLINECROSSINGFINDER_HPP
#define ISOLINECROSSINGFINDER_HPP

#include "GlideSolvers/ZeroFinder.hpp"

class GeoEllipse;
class AATPoint;

class IsolineCrossingFinder:
  public ZeroFinder
{
public:
  IsolineCrossingFinder(const AATPoint& _aap,
                        const GeoEllipse &_ell,
                        const double xmin, 
                        const double xmax);
  double f(const double t);
  bool valid(const double x);
  double solve();
private:
  const GeoEllipse &ell;
  const AATPoint &aap;
};

#endif
