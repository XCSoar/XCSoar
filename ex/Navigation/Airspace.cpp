#include "Airspace.hpp"
#include "AbstractAirspace.hpp"

void 
Airspace::destroy()
{
  if (pimpl_airspace) {
    delete pimpl_airspace;
  }
}

Airspace::Airspace(AbstractAirspace& airspace,
                   const TaskProjection& tp):
  FlatBoundingBox(airspace.get_bounding_box(tp)),
  pimpl_airspace(&airspace)
{

}


bool 
Airspace::inside(const AIRCRAFT_STATE &loc) const
{
  if (pimpl_airspace) {
    return pimpl_airspace->inside(loc);
  } else {
    return false;
  }
}
