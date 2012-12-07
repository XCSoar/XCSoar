/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "TaskMacCreadyTotal.hpp"
#include "TaskSolution.hpp"
#include "Task/Points/TaskPoint.hpp"

TaskMacCreadyTotal::TaskMacCreadyTotal(const std::vector<OrderedTaskPoint*> &_tps,
                                       const unsigned _activeTaskPoint,
                                       const GlideSettings &settings,
                                       const GlidePolar &_gp)
  :TaskMacCready(_tps, _activeTaskPoint, settings, _gp)
{
}

GlideResult
TaskMacCreadyTotal::tp_solution(const unsigned i,
                                const AircraftState &aircraft,
                                fixed minH) const
{
  return TaskSolution::GlideSolutionPlanned(*points[i], aircraft,
                                            settings, glide_polar, minH);
}

const AircraftState &
TaskMacCreadyTotal::get_aircraft_start(const AircraftState &aircraft) const
{
  if (points[0]->HasEntered()) {
    return points[0]->GetEnteredState();
  } else {
    return aircraft;
  }
}

fixed
TaskMacCreadyTotal::effective_distance(const fixed time_remaining) const
{

  fixed t_total = fixed(0);
  fixed d_total = fixed(0);
  for (int i = end_index; i >= start_index; i--) {
    const GlideResult &result = leg_solutions[i];

    if (result.IsOk() && positive(result.time_elapsed)) {
      fixed p = (time_remaining - t_total) / result.time_elapsed;
      if ((p>=fixed(0)) && (p<=fixed(1))) {
        return d_total + p * result.vector.distance;
      }

      d_total += result.vector.distance;
      t_total += result.time_elapsed;
    }
  }
  return d_total;
}

fixed
TaskMacCreadyTotal::effective_leg_distance(const fixed time_remaining) const
{
  const GlideResult &result = get_active_solution();
  fixed p = time_remaining / result.time_elapsed;
  return p * result.vector.distance;
}

