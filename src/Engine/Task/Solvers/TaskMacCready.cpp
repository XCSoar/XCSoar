/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "TaskMacCready.hpp"
#include "TaskSolution.hpp"
#include "Task/Ordered/Points/OrderedTaskPoint.hpp"

#include <algorithm>

TaskMacCready::TaskMacCready(const std::vector<OrderedTaskPoint*> &_tps,
                             const unsigned _active_index,
                             const GlideSettings &_settings,
                             const GlidePolar &gp):
  points(_tps.begin(), _tps.end()),
  leg_solutions(_tps.size()),
  active_index(_active_index),
  start_index(0),
  end_index(max((int)_tps.size(), 1) - 1),
  settings(_settings),
  glide_polar(gp) {}

TaskMacCready::TaskMacCready(TaskPoint* tp, const GlideSettings &_settings,
                             const GlidePolar &gp):
  points(1, tp),
  leg_solutions(1),
  active_index(0),
  start_index(0),
  end_index(0),
  settings(_settings),
  glide_polar(gp) {}

TaskMacCready::TaskMacCready(const std::vector<TaskPoint*> &_tps,
                             const GlideSettings &_settings,
                             const GlidePolar &gp):
  points(_tps.begin(), _tps.end()),
  leg_solutions(_tps.size()),
  active_index(0),
  start_index(0),
  end_index(max((int)_tps.size(), 1) - 1),
  settings(_settings),
  glide_polar(gp) {}

GlideResult 
TaskMacCready::glide_solution(const AircraftState &aircraft) 
{
  const fixed aircraft_min_height = get_min_height(aircraft);
  GlideResult acc_gr;
  AircraftState aircraft_predict = get_aircraft_start(aircraft);

  for (int i = start_index; i <= end_index; ++i) {
    const fixed tp_min_height = std::max(aircraft_min_height,
                                         points[i]->GetElevation());

    // perform estimate, ensuring that alt is above previous taskpoint  
    GlideResult gr = tp_solution(i, aircraft_predict, tp_min_height);
    leg_solutions[i] = gr;

    // update state
    if (i == start_index)
      acc_gr = gr;
    else
      acc_gr.Add(gr);

    /* make sure the next leg doesn't start below the safety altitude
       of the current turn point, because we assume that the pilot
       will never progress to the next leg if he's too low */
    aircraft_predict.altitude = tp_min_height;
    if (positive(gr.altitude_difference))
      /* .. but start higher if the last calculation allows it */
      aircraft_predict.altitude += gr.altitude_difference;
  }

  leg_solutions[active_index].CalcDeferred();
  acc_gr.CalcDeferred();
  return acc_gr;
}

GlideResult 
TaskMacCready::glide_sink(const AircraftState &aircraft, const fixed S)
{
  AircraftState aircraft_predict = aircraft;
  GlideResult acc_gr;

  for (int i = start_index; i <= end_index; ++i) {
    const GlideResult gr = tp_sink(i, aircraft_predict, S);

    aircraft_predict.altitude -= gr.height_glide;
    if (i == start_index)
      acc_gr = gr;
    else
      acc_gr.altitude_difference =
          min(acc_gr.altitude_difference, gr.altitude_difference);
  }

  return acc_gr;
}

GlideResult 
TaskMacCready::tp_sink(const unsigned i,
                       const AircraftState &aircraft, 
                       const fixed S) const
{
  return TaskSolution::GlideSolutionSink(*points[i], aircraft,
                                         settings, glide_polar, S);
}
