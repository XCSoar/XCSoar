#include "ScoredTaskPoint.hpp"

ScoredTaskPoint::ScoredTaskPoint(const TaskProjection& tp,
                                 const Waypoint & wp, 
                                 const TaskBehaviour &tb,
                                 const bool b_scored): 
  SampledTaskPoint(tp, wp, tb, b_scored)
{
    state_entered.Time = -1;
    state_exited.Time = -1;
}

bool 
ScoredTaskPoint::transition_enter(const AIRCRAFT_STATE & ref_now, 
                                  const AIRCRAFT_STATE & ref_last)
{
  bool entered = ObservationZone::transition_enter(ref_now, ref_last);
  if (entered) {
    state_entered = ref_now;
  }
  return entered;
}

bool 
ScoredTaskPoint::transition_exit(const AIRCRAFT_STATE & ref_now, 
                                  const AIRCRAFT_STATE & ref_last)
{
  bool exited = ObservationZone::transition_exit(ref_now, ref_last);
  if (exited) {
    state_exited = ref_last;
  }
  return exited;
}


GEOPOINT 
ScoredTaskPoint::get_reference_travelled() const
{
  if (has_entered()) {
    return getMaxLocation();
  } else {
    return getLocation();
  }
}

GEOPOINT 
ScoredTaskPoint::get_reference_scored() const
{
  return getLocation();
}

GEOPOINT 
ScoredTaskPoint::get_reference_nominal() const
{
  return getLocation();
}

GEOPOINT 
ScoredTaskPoint::get_reference_remaining() const
{
  if (has_entered()) {
    return getMinLocation();
  } else {
    return getLocation();
  }
}


