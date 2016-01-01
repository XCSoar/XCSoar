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

#include "TaskMacCreadyTravelled.hpp"
#include "TaskSolution.hpp"
#include "Task/Points/TaskPoint.hpp"
#include "Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Navigation/Aircraft.hpp"

GlideResult
TaskMacCreadyTravelled::SolvePoint(const TaskPoint &tp,
                                   const AircraftState &aircraft,
                                   double minH) const
{
  assert(tp.GetType() != TaskPointType::UNORDERED);
  const OrderedTaskPoint &otp = (const OrderedTaskPoint &)tp;

  return TaskSolution::GlideSolutionTravelled(otp, aircraft,
                                              settings, glide_polar, minH);
}

AircraftState
TaskMacCreadyTravelled::get_aircraft_start(const AircraftState &aircraft) const
{
  const OrderedTaskPoint &tp = *(const OrderedTaskPoint *)points[0];
  assert(tp.GetType() != TaskPointType::UNORDERED);

  if (tp.HasEntered()) {
    return tp.GetEnteredState();
  } else {
    return aircraft;
  }
}

double
TaskMacCreadyTravelled::get_min_height(const AircraftState &aircraft) const
{
  return aircraft.altitude;
}
