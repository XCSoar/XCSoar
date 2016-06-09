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

#ifndef TASKADVANCE_HPP
#define TASKADVANCE_HPP

#include "Compiler.h"

class TaskPoint;
struct AircraftState;

/**
 * Class used to control advancement through an OrderedTask
 */
class TaskAdvance
{
protected:
  /** arm state */
  bool armed = false;

  /** need to arm */
  bool request_armed = false;

public:

  /**
   * Enumeration of states the task advance mechanism can be in
   */
  enum State {
    /** Advance is manual (user must manually adjust) */
    MANUAL = 0,
    /** Advance is auto (no user input required) */
    AUTO,
    /** Armed for start */
    START_ARMED,
    /** Not yet armed for start (user must arm when ready) */
    START_DISARMED,
    /** Armed for a turn */
    TURN_ARMED,
    /** Not yet armed for turn (user must arm when ready) */
    TURN_DISARMED
  };

  /**
   * Resets as if never flown
   */
  virtual void Reset();

  /**
   * Set arming trigger
   *
   * @param do_armed True to arm trigger, false to clear
   */
  void SetArmed(const bool do_armed);

  /**
   * Accessor for arm state
   *
   * @return True if armed
   */
  bool IsArmed() const {
    return armed;
  }

  /**
   * Accessor for arm request state
   *
   * @return True if arm requested
   */
  bool NeedToArm() const  {
    return request_armed;
  }

  /**
   * Toggle arm state
   *
   * @return Arm state after toggle
   */
  bool ToggleArmed();

  /** 
   * Retrieve current advance state
   * 
   * @return Advance state
   */
  virtual State GetState() const = 0;

  /**
   * Determine whether all conditions are satisfied for a turnpoint
   * to auto-advance based on condition of the turnpoint, transition
   * characteristics and advance mode.
   *
   * @param tp The task point to check for satisfaction
   * @param state current aircraft state
   * @param x_enter whether this step transitioned enter to this tp
   * @param x_exit whether this step transitioned exit to this tp
   *
   * @return true if this tp is ready to advance
   */
  virtual bool CheckReadyToAdvance(const TaskPoint &tp,
                                   const AircraftState &state,
                                   const bool x_enter, const bool x_exit) = 0;

protected:
  /** 
   * Update state after external change to the arm state
   */
  virtual void UpdateState() = 0;

  /**
   * Determine whether, according to OZ entry, an AAT OZ is ready to advance
   *
   * @param has_entered True if the aircraft has entered the OZ
   * @param close_to_target If true, will advance when aircraft is close to target
   *
   * @return True if ready to advance
   */
  virtual bool IsAATStateReady(const bool has_entered,
                               const bool close_to_target) const;

  /**
   * Determine whether state is satisfied for a turnpoint
   *
   * @param tp The task point to check for satisfaction
   * @param state current aircraft state
   * @param x_enter whether this step transitioned enter to this tp
   * @param x_exit whether this step transitioned exit to this tp
   *
   * @return true if this tp is ready to advance
   */
  gcc_pure
  bool IsStateReady(const TaskPoint &tp,
                    const AircraftState &state,
                    const bool x_enter,
                    const bool x_exit) const;
};


#endif
