#include "AATIsolineIntercept.hpp"

AATIsolineIntercept::AATIsolineIntercept(const AATPoint& ap):
  AATIsoline(ap)
{
}


bool 
AATIsolineIntercept::intercept(const AATPoint &ap,
                               const AIRCRAFT_STATE &state,
                               const double bearing_offset,
                               GEOPOINT& ip) const
{
  // TODO: adjust for bearing_offset

  AIRCRAFT_STATE s1, s2;
  if (ell.intersect_extended(state.Location,
                             s1.Location, s2.Location)) 
  {
    if (ap.isInSector(s1)) {
      ip = s1.Location;
      return true;
    }
    if (ap.isInSector(s2)) {
      ip = s2.Location;
      return true;
    }
  }
  return false;
}

