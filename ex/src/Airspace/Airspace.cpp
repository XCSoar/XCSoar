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

bool 
Airspace::intersects(const FlatRay& ray) const
{
  return FlatBoundingBox::intersects(ray);
}


bool 
Airspace::intersects(const GEOPOINT& g1, const GeoVector &vec,
  const TaskProjection &tp) const
{
  if (pimpl_airspace) {
    return pimpl_airspace->intersects(g1, vec, tp);
  } else {
    return false;
  }
}


void
Airspace::Accept(BaseVisitor &visitor) const
{
  if (pimpl_airspace) {
    pimpl_airspace->Accept(visitor);
  }
}

