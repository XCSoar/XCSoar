#include "IsolineCrossingFinder.hpp"
#include "Navigation/Geometry/GeoEllipse.hpp"
#include "Task/TaskPoints/AATPoint.hpp"
#include "Util/Tolerances.hpp"

IsolineCrossingFinder::IsolineCrossingFinder(const AATPoint& _aap,
                                             const GeoEllipse &_ell,
                                             const double xmin, 
                                             const double xmax):
  aap(_aap),
  ell(_ell),
  ZeroFinder(xmin, xmax, TOLERANCE_ISOLINE_CROSSING) 
{

}


double 
IsolineCrossingFinder::f(const double t) 
{
  const GEOPOINT a = ell.parametric(t);
  AIRCRAFT_STATE s;
  s.Location = a;
  
  // note: use of isInSector is slow!
  if (aap.isInSector(s)) {
    return 1.0;
  } else {
    return -1.0;
  }
}

bool 
IsolineCrossingFinder::valid(const double x) 
{
  return (f(x)>0) || (f(x+tolerance)>0) || (f(x-tolerance)>0);
}

double 
IsolineCrossingFinder::solve() 
{
  const double sol = find_zero((xmax+xmin)/2);
  if (valid(sol)) {
    return sol;
  } else {
    return -1.0;
  }
}
