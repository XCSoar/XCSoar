#include "TaskPoint.hpp"

const GEOPOINT &
TaskPoint::get_location_remaining() const
{
  return get_location();
}


const GeoVector 
TaskPoint::get_vector_remaining(const AIRCRAFT_STATE &ref) const
{
  return GeoVector(ref.Location, get_location_remaining());
}


double 
TaskPoint::get_elevation() const
{
  return m_elevation+m_task_behaviour.safety_height_arrival;
}

//// These are dummies, never get called usually

const GeoVector 
TaskPoint::get_vector_planned() const
{
  return GeoVector(0,0);
}

const GeoVector 
TaskPoint::get_vector_travelled(const AIRCRAFT_STATE &ref) const
{
  return GeoVector(0,0);
}

const AIRCRAFT_STATE& 
TaskPoint::get_state_entered() const 
{
  // this should never get called
  static const AIRCRAFT_STATE null_state;
  return null_state;
}
