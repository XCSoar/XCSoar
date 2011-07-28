/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#ifndef TASKADVANCELEGACY_HPP
#define TASKADVANCELEGACY_HPP

#include "TaskAdvance.hpp"

/**
 * Class used to control advancement through an OrderedTask
 */
class TaskAdvanceLegacy: 
  public TaskAdvance
{
public:
/** 
 * Constructor.  Sets defaults to auto-mode
 */
  TaskAdvanceLegacy();

/**
 * Advance mode
 */
  enum TaskAdvanceMode_t {
    ADVANCE_MANUAL =0,          /**< No automatic advance */
    ADVANCE_AUTO,               /**< Automatic, triggers as soon as condition satisfied */
    ADVANCE_ARM,                /**< Requires arming of trigger on each task point */
    ADVANCE_ARMSTART            /**< Requires arming of trigger before start, thereafter works as ADVANCE_AUTO */
  };

  TaskAdvance::TaskAdvanceState_t get_advance_state() const;

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
  bool ready_to_advance(const TaskPoint &tp,
                        const AircraftState &state,
                        const bool x_enter, 
                        const bool x_exit);

/** 
 * Set task advance mode
 * 
 * @param the_mode New task advance mode
 */
  void set_mode(TaskAdvanceMode_t the_mode) {
    mode = the_mode;
  }

/** 
 * Get task advance mode
 * 
 * @return Current task advance mode
 */
  TaskAdvanceMode_t get_mode() const {
    return mode;
  }

protected:

  bool aat_state_ready(const bool has_entered,
                       const bool close_to_target) const;

/** 
 * Determine whether mode allows auto-advance, without
 * knowledge about turnpoint or state characteristics
 *
 * @param tp The task point to check for satisfaction
 * 
 * @return True if this mode allows auto-advance
 */
  bool mode_ready(const TaskPoint &tp) const;

  TaskAdvanceMode_t mode;       /**< acive advance mode */
};


#endif
