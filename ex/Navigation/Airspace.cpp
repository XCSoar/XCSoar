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

void 
Airspace::print(std::ostream &f, const TaskProjection &task_projection) const
{
  if (pimpl_airspace) {
    pimpl_airspace->print(f, task_projection);
  } else {
    FlatBoundingBox::print(f, task_projection);
  }
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
