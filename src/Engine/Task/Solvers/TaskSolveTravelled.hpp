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
#ifndef TASKSOLVETRAVELLED_HPP
#define TASKSOLVETRAVELLED_HPP

#include "TaskMacCreadyTravelled.hpp"
#include "Math/ZeroFinder.hpp"

#include <vector>

/**
 *  Abstract class to solve for travelled time.
 */
class TaskSolveTravelled : protected ZeroFinder {
  const AircraftState &aircraft;
  double inv_dt;
  double dt;

protected:
  TaskMacCreadyTravelled tm; /**< Travelled calculator */

public:
  /**
   * Constructor for ordered task points
   *
   * @param tps Vector of ordered task points comprising the task
   * @param activeTaskPoint Current active task point in sequence
   * @param _aircraft Current aircraft state
   * @param gp Glide polar to copy for calculations
   * @param xmin Min value of search parameter
   * @param xmax Max value of search parameter
   */
  TaskSolveTravelled(const std::vector<OrderedTaskPoint *> &tps,
                     unsigned activeTaskPoint,
                     const AircraftState &_aircraft,
                     const GlideSettings &settings, const GlidePolar &gp,
                     double xmin,
                     double xmax);

protected:
  /**
   * Calls travelled calculator
   *
   * @return Time error
   */
  double time_error();

public:
  /**
   * Search for parameter value.
   *
   * @param ce Default parameter value
   *
   * @return Value producing same travelled time
   */
  double search(double ce);
};

#endif
