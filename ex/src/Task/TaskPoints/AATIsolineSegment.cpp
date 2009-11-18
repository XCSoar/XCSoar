#include "AATIsolineSegment.hpp"
#include "Task/Tasks/PathSolvers/IsolineCrossingFinder.hpp"

AATIsolineSegment::AATIsolineSegment(const AATPoint& ap):
  AATIsoline(ap)
{
  IsolineCrossingFinder icf_up(ap, ell, 0.0, 0.5);
  IsolineCrossingFinder icf_down(ap, ell, -0.5, 0.0);

  t_up = icf_up.solve();
  t_down = icf_down.solve();
  if ((t_up<-0.5) || (t_down<-0.5)) {
    t_up = 0.0;
    t_down = 0.0;
    // single solution only
  }
}

bool
AATIsolineSegment::valid() const
{
  return (t_up>t_down);
}

GEOPOINT 
AATIsolineSegment::parametric(const double t) const
{
  const double r = t*(t_up-t_down)+t_down;
  return ell.parametric(r);
}
