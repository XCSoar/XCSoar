#include "AATIsolineIntercept.hpp"

AATIsolineIntercept::AATIsolineIntercept(const AATPoint& ap,
                                         const AIRCRAFT_STATE &state):
  AATIsoline(ap),
  p_aircraft(state.Location),
  p_previous(ap.get_previous()->get_reference_remaining()),
  p_target(ap.getTargetLocation())
{


}


bool 
AATIsolineIntercept::intercept(const double bearing_offset,
                               GEOPOINT& ip) const
{

  return true;

}
