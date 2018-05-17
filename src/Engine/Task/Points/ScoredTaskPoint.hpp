/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
 */

#ifndef SCOREDTASKPOINT_HPP
#define SCOREDTASKPOINT_HPP

#include "SampledTaskPoint.hpp"
#include "Navigation/Aircraft.hpp"
#include "Compiler.h"

/**
 * Abstract specialisation of SampledTaskPoint to manage scoring
 * of progress along a task.  To do this, this class keeps track
 * of the aircraft state at entry and exit of the observation zone,
 * and provides methods to retrieve various reference locations used
 * in scoring calculations.
 *
 * \todo 
 * - better documentation of this class!
 */
class ScoredTaskPoint : public SampledTaskPoint
{
  AircraftState state_entered;
  bool has_exited;

public:
  /**
   * Constructor.  Clears entry/exit states on instantiation.
   *
   * @param b_scored Whether distance within OZ is scored
   *
   * @return Partially initialised object
   */
  ScoredTaskPoint(const GeoPoint &location, bool b_scored);

  const GeoPoint &GetLocationRemaining() const {
    return GetLocationMin();
  }

  /**
   * Check whether aircraft has entered the observation zone.
   *
   * @return True if observation zone has been entered
   */
  bool HasEntered() const {
    return state_entered.time > 0;
  }

  /**
   * Recall aircraft state where it entered the observation zone.
   *
   * @return State at entry, or null if never entered
   */
  const AircraftState &GetEnteredState() const {
    return state_entered;
  }

  virtual void Reset();

  /**
   * Test whether aircraft has exited the OZ
   *
   * @return True if aircraft has exited the OZ
   */
  bool HasExited() const {
    return has_exited;
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
  bool TransitionEnter(const AircraftState &ref_now,
                       const AircraftState &ref_last);

  /**
   * Test whether aircraft has exited observation zone and
   * was previously inside; records this transition.
   *
   * @param ref_now State current
   * @param ref_last State at last sample
   *
   * @return True if observation zone is exited now
   */
  bool TransitionExit(const AircraftState &ref_now,
                      const AircraftState &ref_last,
                      const FlatProjection &projection);

  /** Retrieve location to be used for the scored task. */
  gcc_pure
  const GeoPoint &GetLocationScored() const;

  /**
   * Retrieve location to be used for the task already travelled.
   * This is always the scored best location for prior-active task points.
   */
  gcc_pure
  const GeoPoint &GetLocationTravelled() const {
    return GetLocationMin();
  }

protected:
  /**
   * Check if aircraft has transitioned to inside sector
   *
   * @param ref_now Current aircraft state
   * @param ref_last Previous aircraft state
   *
   * @return True if aircraft now inside (and was outside)
   */
  gcc_pure
  virtual bool CheckEnterTransition(const AircraftState &ref_now,
                                    const AircraftState &ref_last) const = 0;

  /**
   * Check if aircraft has transitioned to outside sector
   *
   * @param ref_now Current aircraft state
   * @param ref_last Previous aircraft state
   *
   * @return True if aircraft now outside (and was inside)
   */
  gcc_pure
  virtual bool CheckExitTransition(const AircraftState &ref_now,
                                   const AircraftState &ref_last) const = 0;

private:
  gcc_pure
  virtual bool EntryPrecondition() const {
    return true;
  }

  gcc_pure
  virtual bool ScoreLastExit() const {
    return false;
  }

  gcc_pure
  virtual bool ScoreFirstEntry() const {
    return false;
  }
};

#endif
