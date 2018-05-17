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

#ifndef XCSOAR_SMART_TASK_ADVANCE_HPP
#define XCSOAR_SMART_TASK_ADVANCE_HPP

#include "TaskAdvance.hpp"

/** Class used to control advancement through an OrderedTask */
class SmartTaskAdvance final : public TaskAdvance {
  /** active advance state */
  State state;

public:
  /** 
   * Constructor.  Sets defaults to auto-mode
   */
  SmartTaskAdvance();

  virtual State GetState() const;

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
                                   const bool x_enter, const bool x_exit);

protected:
  virtual void UpdateState();
};

#endif
