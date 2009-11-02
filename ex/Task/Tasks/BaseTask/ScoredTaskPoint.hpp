#ifndef SCOREDTASKPOINT_HPP
#define SCOREDTASKPOINT_HPP

#include "SampledTaskPoint.h"

class ScoredTaskPoint:
  public SampledTaskPoint
{
public:
  ScoredTaskPoint(const TaskProjection& tp,
                  const Waypoint & wp, 
                  const bool b_scored): 
    SampledTaskPoint(tp, wp, b_scored)
  {
    state_entered.Time = -1;
    state_exited.Time = -1;
  };

  virtual ~ScoredTaskPoint() {};

  bool has_entered() const {
    return state_entered.Time>0;
  }
  AIRCRAFT_STATE get_state_entered() const {
    return state_entered;
  }
  void set_state_entered(const AIRCRAFT_STATE& state) {
    state_entered = state;
  }

  virtual bool transition_enter(const AIRCRAFT_STATE & ref_now, 
                                const AIRCRAFT_STATE & ref_last);

  virtual bool transition_exit(const AIRCRAFT_STATE & ref_now, 
                               const AIRCRAFT_STATE & ref_last);

  virtual GEOPOINT get_reference_nominal() const;

  virtual GEOPOINT get_reference_scored() const;

  virtual GEOPOINT get_reference_travelled() const;

  virtual GEOPOINT get_reference_remaining() const;

private:
  AIRCRAFT_STATE state_entered;
  AIRCRAFT_STATE state_exited;
};

#endif
