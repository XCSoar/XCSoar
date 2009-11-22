#ifndef SCOREDTASKPOINT_HPP
#define SCOREDTASKPOINT_HPP

#include "SampledTaskPoint.hpp"

/**
 * Abstract specialisation of SampledTaskPoint to manage scoring
 * of progress along a task.  To do this, this class keeps track
 * of the aircraft state at entry and exit of the observation zone,
 * and provides methods to retrieve various reference locations used
 * in scoring calculations.
 *
 * \todo 
 * - implement reset() method
 * - better documentation of this class!
 */
class ScoredTaskPoint:
  public SampledTaskPoint
{
public:
/** 
 * Constructor.  Clears entry/exit states on instantiation.
 * 
 * @param tp Projection used for internal representations
 * @param wp Waypoint associated with the task point
 * @param tb Task Behaviour defining options (esp safety heights)
 * @param b_scored Whether distance within OZ is scored 
 * 
 * @return Partially initialised object
 */
  ScoredTaskPoint(const TaskProjection& tp,
                  const Waypoint & wp, 
                  const TaskBehaviour &tb,
                  const bool b_scored);

  virtual ~ScoredTaskPoint() {};

  /** 
   * Reset the task (as if never flown)
   * 
   */
  virtual void reset();

/** 
 * Test whether aircraft has entered the OZ
 * 
 * @return True if aircraft has entered the OZ
 */
  bool has_entered() const {
    return state_entered.Time>0;
  }

/** 
 * Test whether aircraft has exited the OZ
 * 
 * @return True if aircraft has exited the OZ
 */
  bool has_exited() const {
    return state_exited.Time>0;
  }

/** 
 * Get entry state of aircraft
 * 
 * @return State on entry
 */
  AIRCRAFT_STATE get_state_entered() const {
    return state_entered;
  }

/** 
 * Test whether aircraft has entered observation zone and
 * was previously outside; records this transition.
 * 
 * @param ref_now State current
 * @param ref_last State at last sample
 * 
 * @return True if observation zone is entered now
 */
  virtual bool transition_enter(const AIRCRAFT_STATE & ref_now, 
                                const AIRCRAFT_STATE & ref_last);

/** 
 * Test whether aircraft has exited observation zone and
 * was previously inside; records this transition.
 * 
 * @param ref_now State current
 * @param ref_last State at last sample
 * 
 * @return True if observation zone is exited now
 */
  virtual bool transition_exit(const AIRCRAFT_STATE & ref_now, 
                               const AIRCRAFT_STATE & ref_last);

/** 
 * Retrieve location to be used for nominal task.
 * This is always the reference location for post-active
 * 
 * @return Location 
 */
  virtual GEOPOINT get_reference_nominal() const;

/** 
 * Retrieve location to be used for the scored task.
 * 
 * @return Location 
 */
  virtual GEOPOINT get_reference_scored() const;

/** 
 * Retrieve location to be used for the task already travelled.
 * This is always the scored best location for prior-active task points.
 * 
 * @return Location 
 */
  virtual GEOPOINT get_reference_travelled() const;

/** 
 * Retrieve location to be used for remaining task.
 * This is either the reference location or target for post-active,
 * or scored best location for prior-active task points.
 * 
 * @return Location 
 */
  virtual GEOPOINT get_reference_remaining() const;

protected:

/** 
 * Set OZ entry state
 * 
 * @param state State at entry
 */
  void set_state_entered(const AIRCRAFT_STATE& state) {
    state_entered = state;
  }

/** 
 * Set OZ exit state
 * 
 * @param state State at exit
 */
  void set_state_exited(const AIRCRAFT_STATE& state) {
    state_exited = state;
  }

private:
  AIRCRAFT_STATE state_entered;
  AIRCRAFT_STATE state_exited;
};

#endif
