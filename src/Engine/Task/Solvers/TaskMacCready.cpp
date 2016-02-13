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

#include "TaskMacCready.hpp"
#include "TaskSolution.hpp"
#include "Task/Points/TaskPoint.hpp"
#include "Navigation/Aircraft.hpp"

#include <algorithm>

GlideResult
TaskMacCready::glide_solution(const AircraftState &aircraft)
{
  const auto aircraft_min_height = get_min_height(aircraft);
  GlideResult acc_gr;
  auto aircraft_predict = get_aircraft_start(aircraft);

  for (unsigned i = 0, size = points.size(); i < size; ++i) {
    const auto tp_min_height = std::max(aircraft_min_height,
                                        points[i]->GetElevation());

    // perform estimate, ensuring that alt is above previous taskpoint
    const auto gr = SolvePoint(*points[i], aircraft_predict, tp_min_height);
    leg_solutions[i] = gr;

    // update state
    if (i == 0)
      acc_gr = gr;
    else
      acc_gr.Add(gr);

    /* make sure the next leg doesn't start below the safety altitude
       of the current turn point, because we assume that the pilot
       will never progress to the next leg if he's too low */
    aircraft_predict.altitude = tp_min_height;
    if (gr.altitude_difference > 0)
      /* .. but start higher if the last calculation allows it */
      aircraft_predict.altitude += gr.altitude_difference;
  }

  leg_solutions[active_index].CalcDeferred();
  acc_gr.CalcDeferred();
  return acc_gr;
}

GlideResult
TaskMacCready::glide_sink(const AircraftState &aircraft, const double S) const
{
  auto aircraft_predict = aircraft;
  GlideResult acc_gr;

  for (unsigned i = 0, size = points.size(); i < size; ++i) {
    const auto gr =
      TaskSolution::GlideSolutionSink(*points[i], aircraft_predict,
                                      settings, glide_polar, S);

    aircraft_predict.altitude -= gr.height_glide;
    if (i == 0)
      acc_gr = gr;
    else
      acc_gr.altitude_difference =
        std::min(acc_gr.altitude_difference, gr.altitude_difference);
  }

  return acc_gr;
}
