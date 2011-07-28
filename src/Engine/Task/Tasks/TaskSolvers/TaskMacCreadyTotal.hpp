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
#ifndef TASKMACCREADYTOTAL_HPP
#define TASKMACCREADYTOTAL_HPP

#include "TaskMacCready.hpp"

/** 
 * Specialisation of TaskMacCready for total task
 */
class TaskMacCreadyTotal: 
  public TaskMacCready
{
public:
/** 
 * Constructor for ordered task points
 * 
 * @param _tps Vector of ordered task points comprising the task
 * @param _activeTaskPoint Current active task point in sequence
 * @param _gp Glide polar to copy for calculations
 */
  TaskMacCreadyTotal(const std::vector<OrderedTaskPoint*> &_tps,
                     const unsigned _activeTaskPoint,
                     const GlidePolar &_gp);

/** 
 * Calculate effective distance remaining such that at the virtual
 * point, the time remaining is the same as the reference time
 * remaining (for whole task)
 * 
 * @param time_remaining Time remaining (s) 
 * 
 * @return Effective distance remaining (m)
 */
  fixed effective_distance(const fixed time_remaining) const;

/** 
 * Calculate effective distance remaining such that at the virtual
 * point, the time remaining is the same as the reference time
 * remaining (for active leg)
 * 
 * @param time_remaining Time remaining (s) for active leg
 * 
 * @return Effective distance remaining (m) for active leg
 */
  fixed effective_leg_distance(const fixed time_remaining) const;

private:
  virtual GlideResult tp_solution(const unsigned i,
                                  const AircraftState &aircraft, 
                                  fixed minH) const;
  virtual fixed get_min_height(const AircraftState &aircraft) const {
    return fixed_zero;
  }
  virtual const AircraftState get_aircraft_start(const AircraftState &aircraft) const;

};


#endif
