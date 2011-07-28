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
#ifndef TASKMACCREADYTRAVELLED_HPP
#define TASKMACCREADYTRAVELLED_HPP

#include "TaskMacCready.hpp"

/** 
 * Specialisation of TaskMacCready for task travelled
 */
class TaskMacCreadyTravelled: 
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
  TaskMacCreadyTravelled(const std::vector<OrderedTaskPoint*> &_tps,
                         const unsigned _activeTaskPoint,
                         const GlidePolar &_gp);

private:
  virtual GlideResult tp_solution(const unsigned i,
                                   const AircraftState &aircraft, 
                                   fixed minH) const;
  virtual fixed get_min_height(const AircraftState &aircraft) const;

  virtual const AircraftState get_aircraft_start(const AircraftState &aircraft) const;
};

#endif

