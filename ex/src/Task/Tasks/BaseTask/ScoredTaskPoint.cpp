#include "ScoredTaskPoint.hpp"

ScoredTaskPoint::ScoredTaskPoint(const TaskProjection& tp,
                                 const Waypoint & wp, 
                                 const TaskBehaviour &tb,
                                 const bool b_scored): 
  SampledTaskPoint(tp, wp, tb, b_scored)
{
  reset();
}

bool 
ScoredTaskPoint::transition_enter(const AIRCRAFT_STATE & ref_now, 
                                  const AIRCRAFT_STATE & ref_last)
{
  bool entered = ObservationZone::transition_enter(ref_now, ref_last);
  if (entered) {
    if (!score_first_entry() || !has_entered()) {
      m_state_entered = ref_now;
      return true;
    }
  }
  return entered;
}

bool 
ScoredTaskPoint::transition_exit(const AIRCRAFT_STATE & ref_now, 
                                  const AIRCRAFT_STATE & ref_last)
{
  bool exited = ObservationZone::transition_exit(ref_now, ref_last);
  if (exited) {
    if (score_last_exit()) {
      clear_sample_all_but_last(ref_last);
      m_state_entered = ref_last;
      m_state_exited = ref_now;
     } else {
      m_state_exited = ref_last;
    }
  }
  return exited;
}


const GEOPOINT &
ScoredTaskPoint::get_location_travelled() const
{
  if (has_entered()) {
    return get_location_max();
  } else {
    return get_location();
  }
}

const GEOPOINT &
ScoredTaskPoint::get_location_scored() const
{
  return get_location();
}

const GEOPOINT &
ScoredTaskPoint::get_location_remaining() const
{
  if (has_entered()) {
    return get_location_min();
  } else {
    return get_location();
  }
}


void 
ScoredTaskPoint::reset()
{
  SampledTaskPoint::reset();
  m_state_entered.Time = -1;
  m_state_exited.Time = -1;
}
